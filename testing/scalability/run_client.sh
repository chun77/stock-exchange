#!/bin/bash

# Number of times the client program should run
RUN_COUNT=10

# client executable
CLIENT="./client"

# Run the client program multiple times
for ((i=0; i<$RUN_COUNT; i++))
do
    $CLIENT
done
