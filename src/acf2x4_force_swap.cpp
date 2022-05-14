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
#include <thread>
#include <mutex>
#include <pthread.h>

bool quiet=false;
//int verbose=0; // define the debug level
int seed;

//d=2, b=4 (second variant)
int num_way=2;      //# of ways (hash functions)
int num_cells=4;  //# of slots in a rows
int ht_size=1024; //# of rows
int f=9; //# of fingerprint bits

int ratio_external_attack=0; // Number of external queries for every attack query during the construction of the filter

int max_loop=2;    //num of trials
int load_factor=95;    //load factor


int total_groups = 1;
int64_t tot_access=0;
int max_triggered = 16;

// side effect. just for debug
int first_key = 0;
int current_way = -1;
int current_bucket = 0;

map<int64_t,int> S_map;
map<int64_t,int> A_map;


std::mutex acf_check_mtx;


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
	bool external_can_check = false;

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
        	std::lock_guard<std::mutex> guard(acf_check_mtx);
		if(!quiet) printf("Clearing cuckoo table \n");
           	cuckoo->clear();
		if(!quiet) printf("Clearing additional structure \n");
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
            std::lock_guard<std::mutex> guard(acf_check_mtx);
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


	bool check_ext(int key, int thread){
        	std::lock_guard<std::mutex> guard(acf_check_mtx);
		if(!external_can_check){
			if(!quiet) printf("External disabled\n");
		}
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
				                        if (!quiet) printf("External: false_ii is free\n");
                    				}
                    				if (!cuckoo->remove(key2)) {// jj is free
                        				FF[i][ii][p]=-1;
                    				} else {
                        				cuckoo->direct_insert(key2,value2,i,ii);
                        				FF[i][ii][p]=fingerprint(key2,ii,fbhs);
                    				}
                    				cuckoo->direct_insert(key1,value1,i,jj);
						if(!quiet){
                                               		if (current_way == -1 && key==first_key) {
                                               	       	 current_way = i;
                                               	       	 current_bucket=p;
                                               		}
                                               		if ((i==current_way && p==current_bucket) || (key==first_key) ){
								       	printf("External %u: Key %u Swapping [%u][%u][%u], %u --> %u\n", thread, key, i, ii, p, ii, jj);
							}
                                        	}
                                                FF[i][jj][p]=fingerprint(key1,jj,fbhs);
						//verprintf("Key %u Swapping [%u][%u][%u] --> %u to %u\n", key, i, ii, p, ii, jj);
					}
					return true;
				}
                        }
                }
		return false;
	}

	bool check(int key)
	{
        	std::lock_guard<std::mutex> guard(acf_check_mtx);
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
				                        if (!quiet) printf("false_ii is free\n");
                    				}
                    				if (!cuckoo->remove(key2)) {// jj is free
                        				FF[i][ii][p]=-1;
                    				} else {
                        				cuckoo->direct_insert(key2,value2,i,ii);
                        				FF[i][ii][p]=fingerprint(key2,ii,fbhs);
                    				}
                    				cuckoo->direct_insert(key1,value1,i,jj);
						if(!quiet){
                                               		if (current_way == -1 && key==first_key) {
                                               	       	 current_way = i;
                                               	       	 current_bucket=p;
                                               		}
                                               		if ((i==current_way && p==current_bucket) || (key==first_key) ){
								       	printf("Attacker: Key %u Swapping [%u][%u][%u], %u --> %u\n", key, i, ii, p, ii, jj);
							}
                                        	}
                                                FF[i][jj][p]=fingerprint(key1,jj,fbhs);
						//verprintf("Key %u Swapping [%u][%u][%u] --> %u to %u\n", key, i, ii, p, ii, jj);
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

	void enable_external_checks(bool enable){
		external_can_check = enable;
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

void mutex_check(ACF acf_cuckoo, unsigned int key, int thread){
	acf_cuckoo.check_ext(key, thread);
}

void thread_external_query(ACF acf_cuckoo, int thread){
	int iseed=time(NULL)+thread;
	srand(iseed);
    
    	std::mt19937 gen(iseed);
	std::uniform_int_distribution<> dis(1,INT_MAX);
	// Run a number of external queries from other users between 2 attacker queries.
	while (true){
		unsigned int external_key = (unsigned int) dis(gen);
		mutex_check(acf_cuckoo, external_key, thread);
	}
}


int fact(int n){
	if(n<0){return -1;}
	return (n==0)?1:fact(n-1)*n;
}

bool pick_four(int configuration, vector<int> current_set, vector<int>* new_set){
	int size = current_set.size();
	verprintf("Configuration: %u\n",configuration);
	if(configuration <= 1){ 
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
						return true;
					}
				}
			}
		}
	}
	return false;
}

int run()
{
    
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

    printf("d;c;f;b;time_big_seq;time_reduction;time_total;failure;sequence_size;trigger_condition\n");

//main loop
//
    	bool threads_on = false;
        int num_fails=0;
        int64_t tot_i=(load_factor*acf_cuckoo.get_size())/100;
	std::thread externalThreads[ratio_external_attack];
        for (int loop=0; loop<max_loop; loop++) {
            S_map.clear();
            A_map.clear();
            bool fail_insert=false;

	    acf_cuckoo.enable_external_checks(false);
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
            if (!quiet){ printf("End insertion\n");
            	printf("---------------------------\n");
            	printf("items= %d\n",acf_cuckoo.get_nitem());
            	printf("load(%d)= %f \n",loop,acf_cuckoo.get_nitem()/(0.0+acf_cuckoo.get_size()));
	    }
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
            if (!quiet) printf("1st Consistency passed\n");
	    
	    int total_found = 0;
	    vector<int>* attack_set = new vector<int>[total_groups];
	    vector<double> calculated = {};
	    calculated.resize(total_groups);

            //create and test the false positive sequences
            int n_sequence = 1<<num_cells*2; 

	    acf_cuckoo.enable_external_checks(true);
            // creating threads for external users and mutex
	    if(!threads_on){
	    	for (int th=0; th<ratio_external_attack; th++){
		 	externalThreads[th]=std::thread(thread_external_query, std::ref(acf_cuckoo), th);
			//externalThreads[th].detach();
	    	}
		threads_on = true;
	    }


   	    while(total_found<total_groups){
		time_t starttime = time(NULL);
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
                if (!quiet) printf("inserting false positive key: %u \n", victim_key);
		first_key=victim_key;
		current_way=-1;

		vector<int> current_set = {};
		current_set.resize(n_sequence);
		current_set[0]=victim_key;

		// Activations per element in the set when finding elements
		vector<short> activations = {};
		activations.resize(n_sequence, 0);

		if (!quiet) printf("Looking for sequence %u\n\n",(total_found+1));
		bool failed = true;

		int rr=-1; // To clean possible external false positives from the set using a Round Robin approach

		for(int i=1; i<n_sequence; i++){
               		unsigned int triggering_key = (unsigned int) dis(gen);

			rr = (rr+1)%i; // Index rr of the set that will be traverse using Round Robin
			acf_cuckoo.check(current_set[rr]); // In each iteration, one of the elements of the sequence is cleaned to reduce activations by external users

			if(!is_false_positive(acf_cuckoo, triggering_key, n_sequence)){
				i--;
                    		continue;
			}


			// When detecting it as a false_positive, we have swapped the cell.
			// We will now check if it triggered a false positive on any of the keys in the set,
			int triggered = 0;
			bool end_trigger = false;
			vector<short> it_activ = {};
			it_activ.resize(n_sequence, 0);
			while(!end_trigger && triggered < max_triggered){
				end_trigger=true;
				for (int pos=0; pos<i;pos++){	
					int victim_key = current_set[pos];
					if(acf_cuckoo.check(victim_key)){ 
						triggered++;
						if(!quiet) printf("Triggering with %u over %u\n", triggering_key, victim_key);
						it_activ[pos]++;
						end_trigger=false;
					}
				}
			}
			if (triggered>=std::min((int)ceil((double)i/10),2)){
				current_set[i] = triggering_key;
				if (!quiet) printf("Element found. Element: %u triggered %u elements.\n", triggering_key, triggered);
				std::transform (activations.begin(), activations.end(), it_activ.begin(), activations.begin(), std::plus<int>());
				int removed = 0;
				if (i>10){
					for (int k=i-1; k>=0; k--){
						if (!quiet) printf("Accumulated activation for position: %u is %u.\n", k, activations[k]);
						if (k<i/2 && activations[k]==0){
							// Removing elements with 0 activations
							if (!quiet) printf("Removing element %u from position: %u with activations %u.\n", current_set[k], k, activations[k]);
							current_set.erase(current_set.begin()+k);
							activations.erase(activations.begin()+k);
							current_set.shrink_to_fit();
							activations.shrink_to_fit();
							removed++;
						}
					}
				}
				i = i-removed;
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
			if (!quiet) printf("Retrying sequence %u.\n", total_found+1);
		}else{
			// Removing elements with 0 activations
			int elements = current_set.size();
			for (int aci=elements-1;aci>=0;aci--){
				if (activations[aci]==0){
					current_set.erase(current_set.begin()+aci);
					activations.erase(activations.begin()+aci);
				}
			}
			// new size.
			elements = current_set.size();
			for (int k=0; k<elements;k++){
				if (!quiet) printf("Accumulated activation for position: %u is %u.\n", k, activations[k]);
			}
			attack_set[total_found]=current_set;
			if (!quiet) printf("Sequence completed.\n\n");
			time_t endtime = time(NULL);
			double second = difftime(endtime,starttime);
			calculated[total_found]=second;
			total_found++;
		}

            }

	    vector<bool> errorTuples = {};
	    errorTuples.resize(n_sequence);
	    // Validation of the groups of false positives
            for (int idx=0; idx<total_found; idx++){
		vector<int> current_set = attack_set[idx];
		int elements = current_set.size();
		if (!quiet) printf("Sequence %u with %u elements being checked.\n", idx, elements);

		bool validation = true;
		int swaps = 0;
		while (validation){
			validation=false;
			for (int j=0;j<elements;j++){
				// We do not include external queries in the validation as we are just checking that they are correct for 
				// algorithm validation purposes, not from the attacker's perspective.
				if(acf_cuckoo.check(current_set[j])){
					validation = true;
					swaps++;
					if (!quiet) printf("Element was swapped %u. Is swap %u.\n", current_set[j], swaps);
					break;
				} 
			}
			if (swaps>max_triggered*2){
				break;
			}
		}



		if(validation){
			if (!quiet) printf("Tuple validated: %u, %u", current_set[0], current_set[1]);
			for (int j=2;j<elements;j++){
				if (!quiet) printf(", %u", current_set[j]);
			}
			if (!quiet) printf(".\n");
			errorTuples[idx]=false;
		}else{
			printf("Error in tuple: %u, %u", current_set[0], current_set[1]);
			for (int j=2;j<elements;j++){
				printf(", %u", current_set[j]);
			}
			printf(". Check code.\n");
			errorTuples[idx]=true;
			//exit(1);
		}
            }
	    if(!quiet) fprintf(stderr, "\n");

	    //Reducing elements
            for (int idx=0; idx<total_found; idx++){
		vector<int> current_set = attack_set[idx];
		int elements = current_set.size();

		if(errorTuples[idx]){
			printf("%u;%u;%u;%u;%lf;0;%lf;2;%u;%u\n", num_way, num_cells, f, ht_size, calculated[idx],
				       calculated[idx], elements, max_triggered);
			continue;

		}
		if (!quiet) printf("Sequence %u with %u elements being reduced.\n", idx, elements);

		vector<int> new_set(num_cells);

		time_t starttime = time(NULL);
		bool validation = true;
		int config=0;
		while(pick_four(config+1, current_set, &new_set)){
			if(!quiet){
				printf("Tuple extracted: %u, %u", new_set[0], new_set[1]);
				for (int j=2;j<num_cells;j++){
					printf(", %u", new_set[j]);
				}
				printf(".\n");
			}
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
				if (!quiet) printf("Validated in configuration: %u, %u\n", config+1, (fact(elements)/fact(num_cells)));
				break;
			}
			config++;
		}


		time_t endtime = time(NULL);
		double second = difftime(endtime,starttime);

		// Disable external checks
	        acf_cuckoo.enable_external_checks(false);

		if(validation){
			printf("%u;%u;%u;%u;%lf;%lf;%lf;0;%u;%u\n", num_way, num_cells, f, ht_size, calculated[idx],
				       second, second+calculated[idx], elements, max_triggered);
			if (!quiet){
			       	printf("Tuple validated: %u, %u", new_set[0], new_set[1]);
				for (int j=2;j<num_cells;j++){
					printf(", %u", new_set[j]);
				}
				printf(".\n");
			}
		}else{
			printf("%u;%u;%u;%u;%lf;%lf;%lf;1;%u;%u\n", num_way, num_cells, f, ht_size, calculated[idx],
				       second, second+calculated[idx], elements, max_triggered);
			if(!quiet){
				printf("Impossible to reduce the tuple: %u, %u", current_set[0], current_set[1]);
				for (int j=2;j<elements;j++){
					printf(", %u", current_set[j]);
				}
				printf("\n");
			}
		}
            }


        }// end main loop
            
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
   printf(" -e external queries per attacker query\n");
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
                case 'e':
                    flag=1;
                    ratio_external_attack=atoi(argv[1]);
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
