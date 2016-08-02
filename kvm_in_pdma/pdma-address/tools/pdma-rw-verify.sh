#!/bin/sh

if [ $1 = "-h" 2> /dev/null ]; then
    echo "sh pdma-test-all.sh [count [pool]]"
    exit 0
fi

#mesured by KB
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

    ./pdma-rw-reg $device 1 0x0 0x0
    ./pdma-dma-start $device
    sleep 3s

    #enable loop mode
    ./pdma-rw-reg $device 1 0x0 0x100

    blk_sz=`echo $blk | tr -d k`
    total_blk=$((pool_sz / $((blk_sz))))
    ./pdma-read $device -cnt $total_blk

    #start read/write verify
    count=$((count_total / $((blk_sz))))
    pattern=$((i*0x1000))
    ./pdma-write $device -cnt $count -pt $pattern -inc 1 &
    ./pdma-read  $device -cnt $count -pt $pattern -inc 1

    if [ ! $? -eq 0 ]; then
        echo "pdma-test: loop mode failed for block=$blk"
        exit 1
    fi

    wait
    echo "pdma-test: loop mode passed for block=$blk"

    ./pdma-rw-reg $device 1 0x0 0x0
    ./pdma-dma-stop $device
done

echo "pdma-test: all passed"
