#!/bin/bash

cd bin

./serverM &
sleep 1

./serverA &
sleep 1

./serverR &
sleep 1

./serverD &
sleep 1

./client

echo "Launched all servers"
