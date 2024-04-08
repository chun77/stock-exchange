#!/bin/bash

# Number of clients to run simultaneously
CLIENT_COUNT=10

# Path to the first script
SCRIPT="./run_client.sh"

# array to store start times
declare -a START_TIMES

# Start multiple instances of the first script and record start times
for ((i=0; i<$CLIENT_COUNT; i++))
do
    start=$(date +%s%N)
    $SCRIPT &
    START_TIMES[$i]=$start
done

# Wait for all instances to finish
wait

# Calculate run times for each instance and total time
total_time=0
for ((i=0; i<$CLIENT_COUNT; i++))
do
    end=$(date +%s%N)
    start=${START_TIMES[$i]}
    runtime=$((($end - $start)/1000000)) # convert to milliseconds
    total_time=$((total_time + runtime))
    echo "Client $i finished in $runtime milliseconds."
done

# Calculate average run time
average_time=$(($total_time / $CLIENT_COUNT))
echo "Average run time for $CLIENT_COUNT clients: $average_time milliseconds."
