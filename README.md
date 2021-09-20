# ACFAttackSwap
This repository includes the code in C++ of the attacks to the Adaptive Cuckoo Filter by forcing a degradation due to continuous swapping of the hash functions in one or several buckets as used for the paper:


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

The executables for the ACF attack simulation is:

1. acf4x1\_force\_swap

    This executable looks for false positive pairs associated to the same way-bucket of an ACF for buckets with one cell. The code has been configured with 4 tables and 1 cell for bucket. The executable options can be retrieved running:

```
$ ./acf4x1_force_swap -h 
```

# Example

The following example uses an ACF with 4 tables and 1 cell for bucket loaded up to 95%. Each table contains 128 buckets, the number of fingerprint bits is 16.
The simulator will generate 500 false positives to look for 10 pairs that force the swapping.
The experiment will be run 5 times.

```
$ ./acf4x1_force_swap -L 95 -m 128 -f 16 -a 500 -p 10 -l 5
```

