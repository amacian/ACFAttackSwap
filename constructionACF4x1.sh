#/bin/bash
# b selector bits
# m buckets per way
# f fingerprint size + selector bits
# L load
# p number of tuples to select per iteration
# l iterations
# q do not print additional information, only logs
./acf4x1s_optimized -b 1 -m 65536 -f 9 -L 95 -p 1 -l 100 -q > logb1_m65536_f8_L95_100
./acf4x1s_optimized -b 2 -m 65536 -f 10 -L 95 -p 1 -l 100 -q > logb2_m65536_f8_L95_100
./acf4x1s_optimized -b 3 -m 65536 -f 11 -L 95 -p 1 -l 100 -q > logb3_m65536_f8_L95_100
./acf4x1s_optimized -b 4 -m 65536 -f 12 -L 95 -p 1 -l 100 -q > logb4_m65536_f8_L95_100
./acf4x1s_optimized -b 2 -m 1024 -f 10 -L 95 -p 1 -l 100 -q > logb2_m1024_f8_L95_100
./acf4x1s_optimized -b 2 -m 4096 -f 10 -L 95 -p 1 -l 100 -q > logb2_m4096_f8_L95_100
./acf4x1s_optimized -b 2 -m 16384 -f 10 -L 95 -p 1 -l 100 -q > logb2_m16384_f8_L95_100
./acf4x1s_optimized -b 2 -m 262144 -f 10 -L 95 -p 1 -l 100 -q > logb2_m262144_f8_L95_100
./acf4x1s_optimized -b 2 -m 1048576 -f 10 -L 95 -p 1 -l 100 -q > logb2_m1048576_f8_L95_100
./acf4x1s_optimized -b 2 -m 65536 -f 12 -L 95 -p 1 -l 100 -q > logb2_m65536_f10_L95_100
./acf4x1s_optimized -b 2 -m 65536 -f 14 -L 95 -p 1 -l 100 -q > logb2_m65536_f12_L95_100
./acf4x1s_optimized -b 2 -m 65536 -f 16 -L 95 -p 1 -l 100 -q > logb2_m65536_f14_L95_100
./acf4x1s_optimized -b 2 -m 65536 -f 18 -L 95 -p 1 -l 100 -q > logb2_m65536_f16_L95_100
