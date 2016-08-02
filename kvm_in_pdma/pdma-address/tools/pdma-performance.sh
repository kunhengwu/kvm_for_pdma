#!/bin/sh

if [ $1 = "-h" 2> /dev/null ]; then
    echo "sh pdma-test-all.sh [count [pool]]"
    exit 0
fi

#measured by KB
count_total=40000000
if [ ! -z $1 ]; then
    count_total=$1
fi

#measured by KB
pool_sz=131072
if [ ! -z $2 ]; then
    pool_sz=$2
fi

device=/dev/pdma

for blk in 4k 16k 64k; do
    cd ..
    sh pdma-unload.sh
    sh pdma-load.sh block=$blk pool=$((pool_sz))k
    cd tools

    ./pdma-dma-start $device

    blk_sz=`echo $blk | tr -d k`
    count=$((count_total / $((blk_sz))))

    echo "pdma performance starting for block=$blk"

    #start read/write
    ./pdma-write $device -cnt $count &
    ./pdma-read  $device -cnt $count &

    wait
    echo "pdma performance completed for block=$blk"
    ./pdma-dma-stop $device
done

echo "pdma-test: all passed"
