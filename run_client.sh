#!/bin/bash

# concurrent client count
CLIENT_COUNT=10

# client executable
CLIENT="./mockclient"

# start clients
for ((i=0; i<$CLIENT_COUNT; i++))
do
   $CLIENT &
done

# wait for all clients to finish
wait
