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
int bhs=2;

int total_groups = 1; 

int64_t tot_access=0;

map<int64_t,int> S_map;
map<int64_t,int> A_map;


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
					verprintf("Key %u Swapping [%u][%u], %u --> %u\n", key, i, p, ii, pFF[i][p].first);
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

bool is_false_positive(ACF acf_cuckoo, int key, int n_sequence){
	if(!acf_cuckoo.check(key)){ 
		return false;
	}
	for (int i=1; i<n_sequence; i++){
		if(!acf_cuckoo.check(key)){ //False positive detected
			return true;
		}
	}
	return false;
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

	    int total_found = 0; 
            vector<int>* attack_set = new vector<int>[total_groups];

            //create and test the false positive sequences
            int n_sequence = 1<<bhs; 

	    int max_tries = 1000;//n_sequence*1.5;//10;
            while(total_found<total_groups){
                unsigned int victim_key = (unsigned int) dis(gen);
                if (A_map.count(victim_key) > 0)
                {
                    continue;
                }

		//Some false positives will not be detected if they affect more than 1 way.
		//However it is not a problem as we only need a subset of them
		if(!is_false_positive(acf_cuckoo, victim_key, n_sequence)){
                    continue;
		}

                if (S_map.count(victim_key) > 0){
			printf("Consistency check failed, true positive detected %u\n", victim_key);
		}

                //insert in A_map 
                A_map[victim_key] = line++;
                verprintf("insert key: %u \n", victim_key);

		vector<int> current_set = {};
		current_set.resize(n_sequence);
		current_set[n_sequence-1]=victim_key;

		printf("Looking for sequence %u\n\n",(total_found+1));
	        int tries = 1;
		bool failed = false;
            	vector<int> positives = {};
		positives.resize(1);
		unsigned int idx_positives = 0;
		for(int i=1; i<n_sequence; i++){
               		unsigned int triggering_key;
			if(i<3){
				triggering_key = (unsigned int) dis(gen);
			}else{
				if (idx_positives>(positives.size()-1)){
					failed = true;	
					break;
				}
				triggering_key = positives[idx_positives++];
				verprintf("Old positive: %u\n", triggering_key);
			}

			if(!is_false_positive(acf_cuckoo, triggering_key, n_sequence)){
				i--;
                    		continue;
			}

			if (i==2){
				positives.push_back(triggering_key);
			}

			// When detecting it as a false_positive, we have swapped the fingerprint.
			// We will now check if it triggered a false positive on victim_key,
			if(!acf_cuckoo.check(victim_key)){ 
				// As it may have triggered a false positive on a different element
				// of the sequence, let's clean them before continue
				bool retrigger = false;
				for(int pos=n_sequence-i;pos<n_sequence && !retrigger;pos++){
					int next_key = current_set[pos];
					if(acf_cuckoo.check(next_key)){} // trigger next key 
					// We should be careful as one of these keys may be duplicated
					// So we check every step to see if they retrigger the sequence.
					if(acf_cuckoo.check(victim_key)){
						// retriggering in a previous position
						triggering_key = next_key;
						retrigger=true;
						verprintf("Retriggering with %u over %u\n", triggering_key, victim_key);
						break; //stop triggering the rest of the chain as one of them may 
							//also trigger the sequence
					}
				}
				if(!retrigger){
					i--;
                    			continue;
				}
			}


			if (victim_key!=(unsigned int)current_set[n_sequence-i+1] &&
				acf_cuckoo.check(victim_key)){
				// A chained trigger has been detected due to a self-trigger
				verprintf("Chained trigger with %u \n", victim_key);
				triggering_key = victim_key;
			}
			// As the positive may be due to a different fingerprint in a different way
                        // from the one that shared the last two keys, we need to verify that the
                        // sequence is triggered.
			if (i>1 || n_sequence==2){

				verprintf("Candidate for position %u found. Elements: %u triggered %u\n", (n_sequence-i-1), triggering_key, victim_key);
				verprintf("Checking for way-bucket\n");
				int init_pos = n_sequence-i+1;
				if(victim_key==(unsigned int)current_set[n_sequence-i+1] &&
				   acf_cuckoo.check(victim_key)){
					init_pos++;
				}
				if(acf_cuckoo.check(victim_key)){} // victim_key is in position 
				bool same_way_bucket = true;
				bool last_triggers_first = false;
				int prev_key = victim_key;
				for(int pos=init_pos;pos<n_sequence;pos++){
					int next_key = current_set[pos];
					verprintf("Checking if %u triggered %u at position %u --> ", prev_key, next_key, pos);
					if(!acf_cuckoo.check(next_key)){ // Not in the same bucket-way
						verprintf("Failed\n");
						same_way_bucket = false;
						break;
					}
					verprintf("Checked\n");
					// Check for self-triggering
					verprintf("Self-triggering?: %u == %u ", next_key, current_set[(pos==n_sequence-1)?0:pos+1]);
					if(next_key==current_set[(pos==n_sequence-1)?0:pos+1]){
						pos++;
						verprintf("Self-triggering: %u at position %u --> ", next_key, (last_triggers_first)?0:pos);
					}
					if(acf_cuckoo.check(next_key)){} // trigger next key 
					prev_key = next_key;
				}
				if(same_way_bucket && i==(n_sequence-1)){ 
					verprintf("Checking if last element %u triggered candidate %u --> ", prev_key, triggering_key);
					//Once that we have swapped the fingerprint with the last key, 
					// we have to check that the found key is now positive
					// Otherwise, look for a different key
					if(triggering_key==(unsigned int)current_set[n_sequence-1] || acf_cuckoo.check(triggering_key)){ 
						verprintf("Checked\n");
					}else{
						verprintf("Failed\n");
						same_way_bucket = false;
						// We are going to limit the number of tries and then reset the sequence
					}
				}
				if(!same_way_bucket){
					if(tries==max_tries){
						printf("Max number of tries completed for this sequence\n\n\n");
						failed = true;
						break;
					}else{
						verprintf("Tries %u vs. Max number of tries %u\n", tries, max_tries);
						if(i==3){
							//positives[idx_positives-1]=positives[idx_positives];
							//idx_positives = 0;
						}
						/*for(int pos=n_sequence-i;pos<n_sequence;pos++){
						//	while(acf_cuckoo.check(current_set[pos])){} // trigger next key 
						}*/
					}
					tries++;
					i--;
                    			continue;
				}
				tries = 1;
			}
			// Set key 2 in the previous position
			current_set[n_sequence-i-1] = triggering_key;
			verprintf("Element %u found. Elements: %u triggered %u. Iteration: %i; Total_it:%u\n", (n_sequence-i-1), triggering_key, victim_key, i, n_sequence-1);
			victim_key = triggering_key;
			idx_positives = 0;
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

		//Force first elemtent of the sequence to be the next to return a false positive
		for (int j=1;j<n_sequence;j++){
			while(acf_cuckoo.check(current_set[j])){} 
		}

		int prev_key = current_set[n_sequence-1];

		bool validation = true;
		for (int j=0;j<n_sequence;j++){
			int key1 = current_set[j];
			if(acf_cuckoo.check(key1)){} // While Just in case key1 is FP in more than 1 way
			prev_key = key1;
		} 


		if(validation && !(prev_key==current_set[0]) && !acf_cuckoo.check(current_set[0])){
			validation = false;
			printf("Key: %u did not produce a positive after second iteration of swapping key %u. \n", current_set[0], prev_key);
		} 

		if(validation){
			printf("Tuple validated: %u, %u", current_set[0], current_set[1]);
			for (int j=2;j<n_sequence;j++){
				printf(", %u", current_set[j]);
			}
			printf(".\n");
		}else{
			printf("Error in tuple: %u, %u", current_set[0], current_set[1]);
			for (int j=2;j<n_sequence;j++){
				printf(", %u", current_set[j]);
			}
			printf(". Check code.\n");
			exit(1);
		}
            }
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
   printf(" -b b_bits: number of selection bits\n");
  // printf(" -k skewness: skewness factor\n");
   printf(" -n num_packets: number of packets for each flow \n");
   printf(" -S seed: select random seed (for debug)\n");
   printf(" -L load_factor : set the ACF load factor \n");
   printf(" -p total_groups : max number of total groups of FP to be found \n");
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
        // other n-tuples to force the swapping. Although the code
        // can be generalized, it is out of the scope of this experiment.
        while ((c = *++argv[0])){
            switch (c) {
                case 'q':
                    printf("\nQuiet enabled\n");
                    quiet=true;
                    break;
                //case 'k':
                    //flag=1;
                    //printf("Skewed enabled\n");
                    //skewed=atoi(argv[1]);
                    //argc--;
                    //break;
                case 'b':
                    flag=1;
                    bhs=atoi(argv[1]);
                    if(bhs<1){bhs=1;}
                    argc--;
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
                case 'L':
                    flag=1;
                    load_factor=atoi(argv[1]);
                    argc--;
                    break;
                case 'p':
                    flag=1;
                    total_groups=atoi(argv[1]);
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
    printf("iterations: %d\n",max_loop);
    printf("max groups: %d\n",total_groups);
    printf("Load factor: %d\n",load_factor);
    printf("---------------------------\n");


}


int main(int argc, char **argv) {
    init(argc,argv);
    return run();
}
