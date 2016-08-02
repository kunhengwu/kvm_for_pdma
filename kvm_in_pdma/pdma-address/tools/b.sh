#!/bin/bash
for((i=0;i<1000;i=i++))
do
	sudo ./pdma-rw-reg /dev/pdma 1 0 0x0500ffff
done
