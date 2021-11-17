#/bin/bash
# m buckets per way
# f fingerprint size 
# L load
# p number of tuples to select per iteration
# l iterations
# t number of adaptations to be run to consider a continuous adaptation
# q do not print additional information, only logs
./acf2x4_force_swap -m 1024 -f 8 -L 95 -p 1 -l 100 -t 300 -q > log2x4_m1024_f8_L95_t300_100
./acf2x4_force_swap -m 4096 -f 8 -L 95 -p 1 -l 100 -t 300 -q > log2x4_m4096_f8_L95_t300_100
./acf2x4_force_swap -m 16384 -f 8 -L 95 -p 1 -l 100 -t 300 -q > log2x4_m16384_f8_L95_t300_100
./acf2x4_force_swap -m 65536 -f 8 -L 95 -p 1 -l 100 -t 300 -q > log2x4_m65536_f8_L95_t300_100
./acf2x4_force_swap -m 262144 -f 8 -L 95 -p 1 -l 100 -t 300 -q > log2x4_m262144_f8_L95_t300_100
./acf2x4_force_swap -m 1048576 -f 8 -L 95 -p 1 -l 100 -t 300 -q > log2x4_m1048576_f8_L95_t300_100
./acf2x4_force_swap -m 65536 -f 10 -L 95 -p 1 -l 100 -t 300 -q > log2x4_m65536_f10_L95_t300_100
./acf2x4_force_swap -m 65536 -f 12 -L 95 -p 1 -l 100 -t 300 -q > log2x4_m65536_f12_L95_t300_100
./acf2x4_force_swap -m 65536 -f 8 -L 95 -p 1 -l 100 -t 10 -q > log2x4_m65536_f8_L95_t10_100
./acf2x4_force_swap -m 65536 -f 8 -L 95 -p 1 -l 100 -t 20 -q > log2x4_m65536_f8_L95_t20_100
./acf2x4_force_swap -m 65536 -f 8 -L 95 -p 1 -l 100 -t 30 -q > log2x4_m65536_f8_L95_t30_100
./acf2x4_force_swap -m 65536 -f 8 -L 95 -p 1 -l 100 -t 50 -q > log2x4_m65536_f8_L95_t50_100
./acf2x4_force_swap -m 65536 -f 8 -L 95 -p 1 -l 100 -t 100 -q > log2x4_m65536_f8_L95_t100_100

./acf2x4_effect -m 65536 -f 6 -L 95 -p 1 -l 10 -t 300 -q > adaptation2x4_m65536_f6_L95_t300_10_a1
./acf2x4_effect -m 65536 -f 6 -L 95 -p 10 -l 10 -t 300 -q > adaptation2x4_m65536_f6_L95_t300_10_a10
./acf2x4_effect -m 65536 -f 8 -L 95 -p 1 -l 10 -t 300 -q > adaptation2x4_m65536_f8_L95_t300_10_a1
./acf2x4_effect -m 65536 -f 8 -L 95 -p 10 -l 10 -t 300 -q > adaptation2x4_m65536_f8_L95_t300_10_a10
./acf2x4_effect -m 65536 -f 10 -L 95 -p 1 -l 10 -t 300 -q > adaptation2x4_m65536_f10_L95_t300_10_a1
./acf2x4_effect -m 65536 -f 10 -L 95 -p 10 -l 10 -t 300 -q > adaptation2x4_m65536_f10_L95_t300_10_a10

./acf2x4_effect -m 65536 -f 6 -L 95 -p 10 -l 10 -t 300 -a 0.05 -q > adaptation2x4_m65536_f6_L95_t300_10_a10_ap005
./acf2x4_effect -m 65536 -f 8 -L 95 -p 10 -l 10 -t 300 -a 0.05 -q > adaptation2x4_m65536_f8_L95_t300_10_a10_ap005
./acf2x4_effect -m 65536 -f 10 -L 95 -p 10 -l 10 -t 300 -a 0.05 -q > adaptation2x4_m65536_f10_L95_t300_10_a10_ap005
./acf2x4_effect -m 65536 -f 6 -L 95 -p 10 -l 10 -t 300 -a 0.1 -q > adaptation2x4_m65536_f6_L95_t300_10_a10_ap01
./acf2x4_effect -m 65536 -f 8 -L 95 -p 10 -l 10 -t 300 -a 0.1 -q > adaptation2x4_m65536_f8_L95_t300_10_a10_ap01
./acf2x4_effect -m 65536 -f 10 -L 95 -p 10 -l 10 -t 300 -a 0.1 -q > adaptation2x4_m65536_f10_L95_t300_10_a10_ap01
