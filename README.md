# ACFAttackSwap
This repository includes the code in C++ of the attacks to the Adaptive Cuckoo Filter by forcing a degradation due to continuous swapping of the hash functions in one or several buckets as used for the paper:
P. Reviriego, A. Sánchez-Macian, S. Pontarelli, S. Liu and F. Lombardi, "Attacking Adaptive Cuckoo Filters: Too Much Adaptation Can Kill You", under submission to IEEE...

The code is based on the original one for ACF available at:
https://github.com/pontarelli/ACF

# Getting Started

The simulation has been tested on Ubuntu 18.04. Other distributions or versions may need different steps.

# Building

Run the following commands in the ACF directory to build everything:

```
$ make
```

# Running

The scripts that run the executables for the ACF attack simulation are:

1. constructionACF2x4.sh
    Shell script to run the simulations for an ACF filter with 2 tables and 4 cells per bucket in each table.
    Two executables are run with different options: acf2x4\_force\_swap and acf2x4\_effect
    The first executable looks for false positive groups of elements and the second one checks the effect of querying using false positives in the filter.

```
$ ./constructionACF2x4.sh 
```

2. constructionACF4x1.sh
    Shell script to run the simulations for an ACF filter with 4 tables and 1 cell per bucket in each table.
    Three executables are run with different options: acf4x1s\_original, acf4x1s\_optimized and acf4x1s\_effect
    The first and second executable files look for false positive groups of elements using the original and optimized algorithms respectively, while the thrid one checks the effect of querying using false positives in the filter.

```
$ ./constructionACF4x1.sh 
```
# Options and example

The previous scripts use different options to run the files. To understand the meaning, some examples are included next.

The following example uses an ACF with 4 tables and 1 cell per bucket loaded up to 95% (-L 95). Each table contains 1024 buckets (-m 1024), the number of fingerprint functions is 4 (-b 2 => b=2 to the power of 2 =>4), with 8 fingerprint bits (-f 10 minus the 2 bits for functions), omitting part of the output (-q) .
The simulator will run the original algorithm 100 times (-l 100) and it will look for 1 tuple (-p 1) that force the swapping in each run.

```
$ ./acf4x1s_original -b 2 -m 1024 -f 10 -L 95 -p 1 -l 100 -q 
```

The following example uses an ACF with 4 tables and 1 cell per bucket loaded up to 95% (-L 95). Each table contains 65536 buckets (-m 65536), the number of fingerprint functions is 4 (-b 2 => b=2 to the power of 2 =>4), with 6 fingerprint bits (-f 8 minus the 2 bits for functions), omitting part of the output (-q) .
The simulator will run the optimized algorithm 10 times (-l 10) and it will look for 10 tuples (-p 10) that force the swapping in each run. The percentage of queries with the attack tuples will be a 5% (-a 0.05).

```
$ ./acf4x1s_effect -b 2 -m 65536 -f 8 -L 95 -p 10 -l 10 -a 0.05 -q
```

The following example uses an ACF with 2 tables and 4 cells per bucket loaded up to 95% (-L 95). Each table contains 4096 buckets (-m 4096), the number of fingerprint bits is 8 (-f 8), omitting part of the output (-q) .
The simulator will run 100 times (-l 100) and it will look for 1 tuple (-p 1) that force the swapping in each run.
It will consider that the tuple forces continuous adaptation, if the elements of the tuple are able to trigger another element of the tuple at least 300 times.

```
$ ./acf2x4_force_swap -m 4096 -f 8 -L 95 -p 1 -l 100 -t 300 -q
```

The following example uses an ACF with 2 tables and 4 cells per bucket loaded up to 95% (-L 95). Each table contains 65536 buckets (-m 65536), the number of fingerprint bits is 6 (-f 6), omitting part of the output (-q) .
The simulator will run the algorithm 10 times (-l 10) and it will look for 10 tuples (-p 10) that force the swapping in each run. The percentage of queries with the attack tuples will be a 5% (-a 0.05).
It will consider that the tuple forces continuous adaptation, if the elements of the tuple are able to trigger another element of the tuple at least 300 times.

```
$ ./acf2x4_effect -m 65536 -f 6 -L 95 -p 10 -l 10 -t 300 -a 0.05 -q
```
