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

//d=2, b=4 (second variant)
int num_way=2;      //# of ways (hash functions)
int num_cells=4;  //# of slots in a rows
int ht_size=1024; //# of rows
int f=9; //# of fingerprint bits



int max_loop=2;    //num of trials
int load_factor=95;    //load factor


int total_groups = 1;
int64_t tot_access=0;
int max_triggered = 16;

map<int64_t,int> S_map;
map<int64_t,int> A_map;



int fingerprint(int64_t key,int index,int f) {
    return hashg(key,20+index,1<<f);
}

int myrandom (int i) { return std::rand()%i;}

class ACF
{
    protected:
	HTmap<int64_t,int>* cuckoo;
    	int *** FF;
	map<int64_t,int> S_map;
        int fbhs = 9;
	int num_way=2;
	int num_cells=4;
	int buckets=1024;

    public:
	ACF(int nway, int ncells, int ht_size, int t, int f)
	{
		num_way = nway;
		num_cells = ncells;
		buckets = ht_size;
		fbhs = f;
		HTmap<int64_t,int> ck(num_way,num_cells,ht_size,t);
		cuckoo = new HTmap<int64_t,int>(num_way,num_cells,ht_size,t);
		FF= new int**[num_way];
    		for (int i = 0;  i <num_way;  i++) {
			FF[i] = new int*[num_cells];
		        for (int ii = 0;  ii <num_cells;  ii++){
            			FF[i][ii]= new int[ht_size];
        		}

    		}
	}

	void clear()
	{
           	cuckoo->clear();
            	for (int i = 0;  i <num_way;  i++){
                	for (int ii = 0;  ii <num_cells;  ii++){
                    		for (int iii = 0;  iii <ht_size;  iii++){
                        		FF[i][ii][iii]=-1;
                    		}
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
                FF[std::get<1>(res)][std::get<2>(res)][std::get<3>(res)]=fingerprint(x.first,std::get<2>(res),f);
            }
	    return true;
	}

	bool check(int key)
	{
                for (int i = 0; i < num_way; i++) {
                   	int p = hashg(key, i, buckets);
			for (int ii = 0; ii < num_cells; ii++) {
                        	if (fingerprint(key, ii, fbhs) == FF[i][ii][p]) {
                    			int64_t key1= cuckoo->get_key(i,ii,p);
					
					if (key != key1){ // False positive . Check and swap should be in a different thread.
						  // Notice that it may be a true positive in another of the ways
						  // Or false positive in more than one
						// SWAP
						int value1 = cuckoo->query(key1);
						int jj=ii;
						while(jj==ii) jj=std::rand()%num_cells;
						int64_t key2 = cuckoo->get_key(i,jj,p);
						int value2 = cuckoo->query(key2);
						if (!cuckoo->remove(key1)) {// false_ii is free
				                        printf("false_ii is free\n");
                    				}
                    				if (!cuckoo->remove(key2)) {// jj is free
                        				FF[i][ii][p]=-1;
                    				} else {
                        				cuckoo->direct_insert(key2,value2,i,ii);
                        				FF[i][ii][p]=fingerprint(key2,ii,fbhs);
                    				}
                    				cuckoo->direct_insert(key1,value1,i,jj);
                                                FF[i][jj][p]=fingerprint(key1,jj,fbhs);
						verprintf("Key %u Swapping [%u][%u][%u] --> %u to %u\n", key, i, ii, p, ii, jj);
					}
					return true;
				}
                        }
                }
		return false;
	}

	bool check_noswap(int key)
	{
                for (int i = 0; i < num_way; i++) {
                    	int p = hashg(key, i, buckets);
			for (int ii = 0; ii < num_cells; ii++) {
                        	if (fingerprint(key, ii, fbhs) == FF[i][ii][p]) {
					return true;
                        	}
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

bool is_false_positive(ACF acf_cuckoo, int key, int tables){
	if(!acf_cuckoo.check(key)){ 
		return false;
	}
	for (int i=1; i<tables; i++){ //Check it is not a true positive or a FP with too many collisions
		if(!acf_cuckoo.check(key)){ //False positive detected
			return true;
		}
	}
	return false;
}

int fact(int n){
	if(n<0){return -1;}
	return (n==0)?1:fact(n-1)*n;
}

void pick_four(int configuration, vector<int> current_set, vector<int>* new_set){
	int size = current_set.size();
	verprintf("Configuration: %u\n",configuration);
	if(configuration <= 1 || configuration>(fact(size)/144)){ //
		std::copy(current_set.begin(), current_set.begin()+4, new_set->begin());
	}
	int config = 0;
	for (int i=0;i<size-3;i++){
		for (int j=i+1; j<size-2;j++){
			for (int k=j+1; k<size-1;k++){
				for (int m=k+1; m<size;m++){
					config++;
					if(config==configuration){
						verprintf("Setting Configuration: %u: (%u,%u,%u,%u)\n",configuration,i,j,k,m);
						new_set->at(0)=current_set[i];
						new_set->at(1)=current_set[j];
						new_set->at(2)=current_set[k];
						new_set->at(3)=current_set[m];
						return;
					}
				}
			}
		}
	}
}


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
    //HTmap<int64_t,int> cuckoo(num_way,num_cells,ht_size,1000);
    ACF acf_cuckoo(num_way,num_cells,ht_size,1000,f);
 
    printf("\n***Cuckoo table \n");
    printf("***:way: %d\n",num_way);
    printf("***:num_cells: %d\n",num_cells);
    printf("***:Total table size: %d\n",acf_cuckoo.get_size());
    printf("***:---------------------------\n");

    printf("***:ACF:\n");
    printf("***:fingerprint bits: %d\n",f);
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
                //verprintf("insert key: %u \n",key);
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
            printf("1st Consistency passed\n");
	    
	    int total_found = 0;
	    vector<int>* attack_set = new vector<int>[total_groups];

            //create and test the false positive sequences
            int n_sequence = 1<<num_cells*2; 

   	    while(total_found<total_groups){
                unsigned int victim_key = (unsigned int) dis(gen);
		if (A_map.count(victim_key) > 0)
                {
                    continue;
                }

		//Some false positives will not be detected 
		//However it is not a problem as we only need a subset of them
		if(!is_false_positive(acf_cuckoo, victim_key, num_cells)){
                    continue;
		}

                if (S_map.count(victim_key) > 0){
			printf("Consistency check failed, true positive detected %u\n", victim_key);
		}

		//insert in A_map 
                A_map[victim_key] = line++;
                verprintf("insert false positive key: %u \n", victim_key);

		vector<int> current_set = {};
		current_set.resize(n_sequence);
		current_set[0]=victim_key;

		printf("Looking for sequence %u\n\n",(total_found+1));
		bool failed = true;

		for(int i=1; i<n_sequence; i++){
               		unsigned int triggering_key = (unsigned int) dis(gen);

			if(!is_false_positive(acf_cuckoo, triggering_key, n_sequence)){
				i--;
                    		continue;
			}


			// When detecting it as a false_positive, we have swapped the cell.
			// We will now check if it triggered a false positive on any of the keys in the set,
			int triggered = 0;
			bool end_trigger = false;
			while(!end_trigger && triggered < max_triggered){
				end_trigger=true;
				for (int pos=0; pos<i;pos++){	
					int victim_key = current_set[pos];
					if(acf_cuckoo.check(victim_key)){ 
						triggered++;
						verprintf("Triggering with %u over %u\n", triggering_key, victim_key);
						end_trigger=false;
					}
				}
			}
			if (triggered>=std::min(i,2)){
				current_set[i] = triggering_key;
				verprintf("Element found. Element: %u triggered %u elements.\n", triggering_key, triggered);

			}else{
				i--;
				continue;
			}
			if(!end_trigger){ // Set completed
				current_set.resize(i+1);
				current_set.shrink_to_fit();
				failed = false;
				break;
			}
		}
		if (failed){
			printf("Retrying sequence %u.\n", total_found+1);
		}else{
			attack_set[total_found]=current_set;
			total_found++;
			printf("Sequence completed.\n\n");
		}

            }

	    // Validation of the groups of false positives
            for (int idx=0; idx<total_found; idx++){
		vector<int> current_set = attack_set[idx];
		int elements = current_set.size();
		printf("Sequence %u with %u elements being checked.\n", idx, elements);

		bool validation = true;
		int swaps = 0;
		while (validation){
			validation=false;
			for (int j=0;j<elements;j++){
				if(acf_cuckoo.check(current_set[j])){
					validation = true;
					swaps++;
					break;
				} 
			}
			if (swaps>max_triggered*2){
				break;
			}
		}



		if(validation){
			printf("Tuple validated: %u, %u", current_set[0], current_set[1]);
			for (int j=2;j<elements;j++){
				printf(", %u", current_set[j]);
			}
			printf(".\n");
		}else{
			printf("Error in tuple: %u, %u", current_set[0], current_set[1]);
			for (int j=2;j<elements;j++){
				printf(", %u", current_set[j]);
			}
			printf(". Check code.\n");
			exit(1);
		}
            }
	    if(!quiet) fprintf(stderr, "\n");

	    //Reducing elements
            for (int idx=0; idx<total_found; idx++){
		vector<int> current_set = attack_set[idx];
		int elements = current_set.size();
		printf("Sequence %u with %u elements being reduced.\n", idx, elements);

		vector<int> new_set(num_cells);

		bool validation = true;
		for (int config=0; config<(fact(elements)/144); config++){
			pick_four(config+1, current_set, &new_set);
			verprintf("Tuple extracred: %u, %u", new_set[0], new_set[1]);
			for (int j=2;j<num_cells;j++){
				printf(", %u", new_set[j]);
			}
			printf(".\n");
			
			int swaps = 0;
			validation=true;
			while (validation){
				validation=false;
				for (int j=0;j<num_cells;j++){
					if(acf_cuckoo.check(new_set[j])){
						validation = true;
						swaps++;
						break;
					} 
				}
				if (swaps>max_triggered*2){
					break;
				}
			}
			if (validation){
				printf("Validated in configuration: %u, %u\n", config+1, (fact(elements)/fact(num_cells)));
				break;
			}
		}



		if(validation){
			printf("Tuple validated: %u, %u", new_set[0], new_set[1]);
			for (int j=2;j<num_cells;j++){
				printf(", %u", new_set[j]);
			}
			printf(".\n");
		}else{
			printf("Impossible to reduce the tuple: %u, %u", current_set[0], current_set[1]);
			for (int j=2;j<elements;j++){
				printf(", %u", current_set[j]);
			}
			printf(". Check code.\n");
		}
            }


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
   printf(" -n num_packets: number of packets for each flow \n");
   printf(" -a as_ratio: set the A/S ratio \n");
   printf(" -S seed: select random seed (for debug)\n");
   printf(" -L load_factor : set the ACF load factor \n");
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
        while ((c = *++argv[0])){
            switch (c) {
                case 'q':
                    printf("\nQuiet enabled\n");
                    quiet=true;
                    break;
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
                case 't':
                    flag=1;
                    max_triggered=atoi(argv[1]);
                    argc--;
                    break;
                case 'p':
                    flag=1;
                    total_groups=atoi(argv[1]);
                    argc--;
                    break;
                case 'L':
                    flag=1;
                    load_factor=atoi(argv[1]);
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
    //Print general parameters
    printf("general parameters: \n");
    printf("seed: %d\n",seed);
    printf("way: %d\n",num_way);
    printf("num_cells: %d\n",num_cells);
    printf("Table size: %d\n",ht_size);
    printf("iterations: %d\n",max_loop);
    printf("accept max triggered: %d\n",max_triggered);
    printf("---------------------------\n");


}


int main(int argc, char **argv) {
    init(argc,argv);
    return run();
}
