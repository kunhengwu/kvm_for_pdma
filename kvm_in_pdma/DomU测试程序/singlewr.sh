#!/bin/bash
    cd ./test_write
    ./wtbyte $1
    ./encode
    cd ../test_read
    ./read $1
    cd ..
    
