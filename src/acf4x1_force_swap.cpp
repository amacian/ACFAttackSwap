#include "HTmap.hpp"
#include "utils.h"
#include <string.h>
#include <iostream>
#include <vector>
#include <map>
#include <time.h>
#include <limits.h>
#include <random> // http://en.cppreference.com/w/cpp/numeric/random
#include <algorithm>

bool quiet=false;
//int verbose=0; // define the debug level
int seed;

//d=4, b=1 (first variant)
int num_way=4;      //# of ways (hash functions)
int num_cells=1;  //# of slots in a rows
int ht_size=1024; //# of rows
int f=9; //# of fingerprint bits
int fbhs=9;
int skewed=0;

int max_loop=2;    //num of trials
int load_factor=95;    //load factor
int A=0;
int bhs=1;

int total_pairs = 1; 

int64_t tot_access=0;

map<int64_t,int> S_map;
map<int64_t,int> A_map;
vector<int64_t> A_ar;

//char* filename="ip4_as20.txt"; //database filename
//char* filename=NULL; //database filename

//select the fingerprint function
//                                        16-bhs  
int fingerprint(int64_t key,int index,int a) {
    int s=bhs;
    int r=skewed;
    int range= (1<<(a-r+s))*((1<<r)-1); 
    int range2= 1<<(a-r);
    if  (index>0) range=range2;

    if (r==0) 
	return hashg(key,20+index,1<<a); 
    else
        return hashg(key,20+index,range);
}

int myrandom (int i) { return std::rand()%i;}

class ACF
{
    protected:
	HTmap<int64_t,int>* cuckoo;
    	pair<int,int>** pFF;
	map<int64_t,int> S_map;
        int fbhs = 9;
	int bhs = 0;
	int num_way=4;
	int num_cells=1;
	int buckets=1024;
	int skew=0;

    public:
	ACF(int nway, int ncells, int ht_size, int t, int f, int s, int skewed)
	{
		bhs = s;
		num_way = nway;
		num_cells = ncells;
		buckets = ht_size;
		fbhs = f;
		skew = skewed;
		HTmap<int64_t,int> ck(num_way,num_cells,ht_size,t);
		cuckoo = new HTmap<int64_t,int>(num_way,num_cells,ht_size,t);
		pFF= new pair<int,int>*[num_way];
    		for (int i = 0;  i <num_way;  i++) {
            		pFF[i]= new pair<int,int>[ht_size];
    		}
	}

	void clear()
	{
           	cuckoo->clear();
            	for (int i = 0;  i <num_way;  i++) 
		{
                    	for (int iii = 0;  iii <buckets;  iii++)
			{
                        	pFF[i][iii]=make_pair(0,-1);
                    	}
		}

	}
 

	bool insertBulk(map<int64_t,int> S_map)
	{
            for (auto x: S_map) {
                if(!cuckoo->insert(x.first,x.second))
                {
                    verprintf(" Table full (key: %lu)\n",x.first);
		    return false;
                }
            }
            for (auto x: S_map) {
                auto res= cuckoo->fullquery(x.first);
                pFF[std::get<1>(res)][std::get<3>(res)]=make_pair(0,fingerprint(x.first,0,fbhs));
            }
	    return true;
	}

	bool check(int key)
	{
                for (int i = 0; i < num_way; i++) {
                    int p = myhash<int64_t>(key, i, buckets);
                        int ii=pFF[i][p].first;
                        if (fingerprint(key, ii, fbhs) == pFF[i][p].second) {
                    		int64_t key1= cuckoo->get_key(i,0,p);
				if (key != key1){ // False positive . Check and swap should be in a different thread.
						  // Notice that it may be a true positive in other of the ways
						  // Or false positive in more than one
					// SWAP
		    			if (skew>0) 
						pFF[i][p].first= (pFF[i][p].first +1) %((1<<bhs)+1);
                    			else
						pFF[i][p].first= (pFF[i][p].first +1) %(1<<bhs);

                    			pFF[i][p].second=fingerprint(key1,pFF[i][p].first,fbhs);
				}
				return true;
                        }
                }
		return false;
	}

	bool check_noswap(int key)
	{
                for (int i = 0; i < num_way; i++) {
                    int p = myhash<int64_t>(key, i, buckets);
                        int ii=pFF[i][p].first;
                        if (fingerprint(key, ii, fbhs) == pFF[i][p].second) {
				return true;
                        }
                }
		return false;
	}

	int get_size()
	{
		return cuckoo->get_size();
	}

	int get_nitem()
	{
		return cuckoo->get_nitem();
	}

	void stat()
	{
		cuckoo->stat();
	}
};

int run()
{
    
    time_t starttime = time(NULL);
    int line=0;
    
    //seed=1456047300;
    srand(seed);

    // Seed with a real random value, if available
    std::random_device rd;
    
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dis(1,INT_MAX);

    printf("***********************\n\n");
    printf("***:Summary: \n");
    printf("seed: %d\n",seed);
    
    
    

    // create the table;
    // HTmap<int64_t,int> cuckoo(num_way,num_cells,ht_size,1000);
    ACF acf_cuckoo(num_way,num_cells,ht_size,1000,fbhs,bhs,skewed);
    printf("\n***Cuckoo table \n");
    printf("***:way: %d\n",num_way);
    printf("***:num_cells: %d\n",num_cells);
    printf("***:Total table size: %d\n",acf_cuckoo.get_size());
    printf("***:---------------------------\n");


    printf("***:ACF:\n");
    printf("***:fingerprint bits: %d\n",f);
    printf("***:fingerprint (bhs) bits: %d\n",fbhs);
    printf("***:Buckets: %d\n",num_way*num_cells*ht_size);
    printf("***:Total size (bits): %d\n",f*num_way*num_cells*ht_size);
    printf("***:---------------------------\n");

    setbuf(stdout, NULL);

//main loop
//
        int num_fails=0;
        int64_t tot_i=(load_factor*acf_cuckoo.get_size())/100;
        for (int loop=0; loop<max_loop; loop++) {
            S_map.clear();
            A_map.clear();
            A_ar.clear();
            bool fail_insert=false;

            acf_cuckoo.clear();

            for (int64_t i = 0;  i <tot_i;  i++)
            {
                //int64_t key= rand();
                //unsigned int key= (rand()*2^16)+rand();
                unsigned int key=(unsigned int) dis(gen);
                if (S_map.count(key)>0) {
                    i--;
                    continue;
                }

                S_map[key]=line++;
                verprintf("insert key: %u \n",key);
                if ((i%1000)==0) {
                    if (!quiet) fprintf(stderr,"loop: %d item: %lu\r",loop,i);
                }

            }
            if(!acf_cuckoo.insertBulk(S_map)){
                    num_fails++;
		    fail_insert=true;
                    break;
            }
            if (fail_insert) 
            {
		    loop--;
		    continue;
            }
              
            if (!quiet) fprintf(stderr, "\n");
            printf("End insertion\n");
            printf("---------------------------\n");
            printf("items= %d\n",acf_cuckoo.get_nitem());
            printf("load(%d)= %f \n",loop,acf_cuckoo.get_nitem()/(0.0+acf_cuckoo.get_size()));
	    acf_cuckoo.stat();

            // consistency check !!!
            for (auto x: S_map) {
                bool flagFF = false;
		if(acf_cuckoo.check(x.first)){
                            flagFF = true;
                }
                if (!flagFF) {
                    printf("Consistency ERROR 1 \n");
                    exit(1);
                }
            }
            printf("1st Consistency B passed\n");

            //create and test the A set
            for (int64_t i = 0;  i <A; i++) {
                unsigned int key = (unsigned int) dis(gen);
		// The attacker only knows about A, not about S
                if (A_map.count(key) > 0)
                {
                    i--;
                    continue;
                }

		bool false_positive = false;
		if(acf_cuckoo.check(key)){ //A positive is returned. Check if we can identify a false positive
			if(!acf_cuckoo.check(key)){ //False positive detected
				false_positive = true;
			}
		}
		if(!false_positive){
		    i--;
                    continue;
		}
                if (S_map.count(key) > 0){
			printf("Consistency check failed, true positive detected %u\n", key);
		}
                //insert in A_map and in A_ar 
                A_map[key] = line++;
		A_ar.push_back(key);

                verprintf("insert key: %u \n", key);
                if ((i % 1000) == 0) {
			if(!quiet) fprintf(stderr, "loop: %d. Create the A set: %lu\r", loop, i);
                }
            }
            int64_t ar_size=A_ar.size();

	    int total_found = 0; 
            pair<int,int>* attack_set = new pair<int,int>[total_pairs];
            for(int64_t first=0; first<A-1; first++){ //Take the first element and look for a swapping pair
		unsigned int key = A_ar[first];
		
		while(acf_cuckoo.check(key)){} // Force the swap if it is not already done for this element

            	for(int64_t second=first+1; second<A; second++){ //Take the first element and look for a swapping pair
			unsigned int key2 = A_ar[second];
			if(!acf_cuckoo.check(key2)){ // If it does not produce a FP, go to the next element
				continue;
			}
			if(acf_cuckoo.check(key)){ // If it is a pair, it will produce a FP for the first key after swapping
			        printf("Pair found: %u, %u\n", key2, key);
				attack_set[total_found]=make_pair(key2,key);
				total_found++;
				break;
			}
		}

		if (total_found>=total_pairs){
			break;
		}
	    }


	    // Validation of the pairs of false positives
            for (int idx=0; idx<total_found; idx++){
		int key1 = attack_set[idx].first;
		int key2 = attack_set[idx].second;
		
		bool validation = true;

		while(acf_cuckoo.check(key2)){} // While Just in case key2 is FP in more than 1 way

		if(!acf_cuckoo.check(key1)){
			validation = false;
			printf("Key: %u did not produce a positive after swapping key %u. \n", key1, key2);
		} 

		while(acf_cuckoo.check(key1)){} // While Just in case key1 is FP in more than 1 way

		if(!acf_cuckoo.check(key2)){
			validation = false;
			printf("Key: %u did not produce a positive after swapping key %u. \n", key2, key1);
		} 

		if(!acf_cuckoo.check(key1)){
			validation = false;
			printf("Key: %u did not produce a positive after second iteration of swapping key %u. \n", key1, key2);
		} 

		if(validation){
			printf("Pair validated: %u, %u\n", key2, key1);
		}else{
			printf("Error in pair: %u, %u. Check code\n", key2, key1);
			printf("Checking key: %u? %u. \n", key1, acf_cuckoo.check(key1));
			printf("Checking key: %u? %u. \n", key1, acf_cuckoo.check(key1));
			printf("Checking key: %u? %u. \n", key2, acf_cuckoo.check(key2));
			printf("Checking key: %u? %u. \n", key1, acf_cuckoo.check(key1));
			exit(1);
		}
            }
            printf("ACF FP: %lu \n", ar_size);
	    if(!quiet) fprintf(stderr, "\n");


        }// end main loop
            
        printf("---------------------------\n");
        printf("---------------------------\n");
    

        printf("\n");
        simtime(&starttime);
        return 0;
}

void PrintUsage() {
   printf("usage:\n");
   printf(" ***\n");
   printf(" -m tsize: Table size\n");
   printf(" -f f_bits: number of fingerprint bits\n");
  // printf(" -b b_bits: number of selection bits\n");
  // printf(" -k skewness: skewness factor\n");
   printf(" -n num_packets: number of packets for each flow \n");
   printf(" -a as_ratio: set the A/S ratio \n");
   printf(" -S seed: select random seed (for debug)\n");
   printf(" -L load_factor : set the ACF load factor \n");
   printf(" -p total_pairs : max number of total pairs of FP to be found \n");
   printf(" -v : verbose \n");
   printf(" -h print usage\n");
   printf(" -v verbose enabled\n");
}

void init(int argc, char* argv[])
{
    printf("\n===========================================\n");
    printf("Simulator for the Adaptive Cuckoo Filter with 4x1 tables\n");
    printf("Run %s -h for usage\n",argv[0]);
    printf("===========================================\n\n");



    //code_version();
    print_hostname();
    print_command_line(argc,argv); //print the command line with the option
    seed=time(NULL);
    // Check for switches
    while (argc > 1 && argv[1][0] == '-'){
        argc--;
        argv++;
        int flag=0; //if flag 1 there is an argument after the switch
        int c = 0;


	// Notice that we have disabled skew and number of selection bits as they
        // modify the number of fingerprints functions to be used and we would need
        // n-tuples instead of pairs to force the swapping. Although the code
        // can be generalized, it is out of the scope of this experiment.
        while ((c = *++argv[0])){
            switch (c) {
                case 'q':
                    printf("\nQuiet enabled\n");
                    quiet=true;
                    break;
		case 'a':
                    flag=1;
                    A=atoi(argv[1]);
                    argc--;
                    break;
                //case 'k':
                    //flag=1;
                    //printf("Skewed enabled\n");
                    //skewed=atoi(argv[1]);
                    //argc--;
                    //break;
                //case 'b':
                    //flag=1;
                    //bhs=atoi(argv[1]);
                    //argc--;
                    //break;
                case 'm':
                    flag=1;
                    ht_size=atoi(argv[1]);
                    argc--;
                    break;
                case 'f':
                    flag=1;
                    f=atoi(argv[1]);
                    argc--;
                    break;
                case 'S':
                    flag=1;
                    seed=atoi(argv[1]);
                    argc--;
                    break;
                case 'L':
                    flag=1;
                    load_factor=atoi(argv[1]);
                    argc--;
                    break;
                case 'p':
                    flag=1;
                    total_pairs=atoi(argv[1]);
                    argc--;
                    break;
                case 'l':
                    flag=1;
                    max_loop=atoi(argv[1]);
                    argc--;
                    break;
                case 'v':
                    printf("\nVerbose enabled\n");
                    verbose += 1;
                    break;
                case 'h':
                    PrintUsage();
                    exit(1);
                    break;
                default :
                    printf("Illegal option %c\n",c);
                    PrintUsage();
                    exit(1);
                    break;
            }
        }
        argv= argv + flag;
    }
    fbhs=f-bhs;
    //Print general parameters
    printf("general parameters: \n");
    if (skewed>0) {
        printf("Enable skewed fingerprint\n");
        printf("f0 range: %d/%d \n",(1<<skewed)-1,1<<skewed);
    }
    printf("seed: %d\n",seed);
    printf("way: %d\n",num_way);
    printf("num_cells: %d\n",num_cells);
    printf("Table size: %d\n",ht_size);
    printf("bhs: %d\n",bhs);
    printf("A size: %d\n",A);
    printf("iterations: %d\n",max_loop);
    printf("max pairs: %d\n",total_pairs);
    printf("Load factor: %d\n",load_factor);
    printf("---------------------------\n");


}


int main(int argc, char **argv) {
    init(argc,argv);
    return run();
}
