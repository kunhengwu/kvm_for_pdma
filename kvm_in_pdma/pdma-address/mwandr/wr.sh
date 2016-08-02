#!/bin/bash
#i max 768  suit 252
i=5
while [ $i -le 768 ]; do
   ./wtbyte ${i}
   ./encode ${i}
    
    if [ $i -lt 64 ]; then
        let i+=1
    elif [ $i -lt 128 ]; then
        let i+=2
    elif [ $i -lt 256 ]; then
        let i+=4
    elif [ $i -le 768 ]; then
        let i+=8
    fi
#		sleep 0.1
done
