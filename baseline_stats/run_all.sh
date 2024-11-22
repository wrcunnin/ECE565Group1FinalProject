#!/bin/bash



benchmarks=("astar" "cactusADM" "hmmer" "lbm" "leslie3d" "mcf" "milc" "namd" "omnetpp" "povray" "sjeng" "tonto")

output_dir="/home/shay/a/humphr31/565/ECE565Group1FinalProject/m5out"

file="/home/shay/a/humphr31/565/ECE565Group1FinalProject/m5out/stats.txt"

working_dir="/home/shay/a/humphr31/565/ECE565Group1FinalProject"

cd "$working_dir"

for benchmark in "${benchmarks[@]}"; do

    ./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py -b "$benchmark" --cpu-type=MyMinorCPU --maxinsts=1000000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache

    new_name="/home/shay/a/humphr31/565/ECE565Group1FinalProject/m5out/${benchmark}-stats.txt"

    mv "${file}" "${new_name}"

    echo "Renamed to ${new_name}"
done



