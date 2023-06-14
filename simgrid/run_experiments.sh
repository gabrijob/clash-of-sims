#!/bin/bash

# Define the plaforms files for the experiments
platforms=("exp_100mips_platform.xml" "exp_1000mips_platform.xml" )

# Define the number of tasks to be simulated
tasks_nbs=(100 1000 10000)

# Compile source code
make clean
make

# Loop through the arguments and call the binary with each argument
for platfile in "${platforms[@]}"; do
    for task_nb in "${tasks_nbs[@]}"; do
        ./simgrid-simulation.bin "platforms/$platfile" 4 "$task_nb" 20000.0 "circular"
    done
done