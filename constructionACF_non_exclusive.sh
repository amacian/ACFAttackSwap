#/bin/bash
# m buckets per way
# f fingerprint size 
# L load
# p number of tuples to select per iteration
# l iterations
# t number of adaptations to be run to consider a continuous adaptation
# q do not print additional information, only logs
./acf2x4_force_swap -m 1024 -f 8 -L 95 -p 1 -l 10 -t 300 -e 9 -q > log2x4_m1024_f8_L95_t300_10_e9
./acf2x4_force_swap -m 4096 -f 8 -L 95 -p 1 -l 10 -t 300 -e 9 -q > log2x4_m4096_f8_L95_t300_10_e9
./acf2x4_force_swap -m 16384 -f 8 -L 95 -p 1 -l 10 -t 300 -e 9 -q > log2x4_m16384_f8_L95_t300_10_e9
./acf2x4_force_swap -m 65536 -f 8 -L 95 -p 1 -l 10 -t 300 -e 9 -q > log2x4_m65536_f8_L95_t300_10_e9
./acf2x4_force_swap -m 262144 -f 8 -L 95 -p 1 -l 10 -t 300 -e 9 -q > log2x4_m262144_f8_L95_t300_10_e9
./acf2x4_force_swap -m 1048576 -f 8 -L 95 -p 1 -l 10 -t 300 -e 9 -q > log2x4_m1048576_f8_L95_t300_10_e9

./acf4x1s_optimized -b 1 -m 65536 -f 9 -L 95 -p 1 -l 10 -e 9 -q > logb1_m65536_f8_L95_100_e9
./acf4x1s_optimized -b 2 -m 65536 -f 10 -L 95 -p 1 -l 10 -e 9 -q > logb2_m65536_f8_L95_100_e9
./acf4x1s_optimized -b 3 -m 65536 -f 11 -L 95 -p 1 -l 10 -e 9 -q > logb3_m65536_f8_L95_100_e9
./acf4x1s_optimized -b 4 -m 65536 -f 12 -L 95 -p 1 -l 10 -e 9 -q > logb4_m65536_f8_L95_100_e9
./acf4x1s_optimized -b 4 -m 1024 -f 10 -L 95 -p 1 -l 10 -e 9 -q > logb2_m1024_f8_L95_100_e9

./acf4x1s_original -b 1 -m 65536 -f 9 -L 95 -p 1 -l 10 -e 9 -q > logb1_m65536_f8_L95_100_orig_e9
./acf4x1s_original -b 2 -m 65536 -f 10 -L 95 -p 1 -l 10 -e 9 -q > logb2_m65536_f8_L95_100_orig_e9
./acf4x1s_original -b 3 -m 65536 -f 11 -L 95 -p 1 -l 10 -e 9 -q > logb3_m65536_f8_L95_100_orig_e9
./acf4x1s_original -b 4 -m 65536 -f 12 -L 95 -p 1 -l 10 -e 9 -q > logb4_m65536_f8_L95_100_orig_e9
./acf4x1s_original -b 2 -m 1024 -f 10 -L 95 -p 1 -l 10 -e 9 -q > logb2_m1024_f8_L95_100_orig_e9
