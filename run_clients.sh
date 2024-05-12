#!/bin/bash

EXECUTABLE="./kierki-klient -h localhost -p 1234 -W"

NUM_INSTANCES=5

./kierki-klient -h localhost -p 1234 -N &
./kierki-klient -h localhost -p 1234 -E &
./kierki-klient -h localhost -p 1234 -S &
./kierki-klient -h localhost -p 1234 -W &

for ((i=1; i<=$NUM_INSTANCES; i++))
do
    $EXECUTABLE &
done

wait
