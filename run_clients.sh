#!/bin/bash

EXECUTABLE="./kierki-klient -h localhost -p 1234 -W"

NUM_INSTANCES=5

for ((i=1; i<=$NUM_INSTANCES; i++))
do
    $EXECUTABLE &
done

wait
