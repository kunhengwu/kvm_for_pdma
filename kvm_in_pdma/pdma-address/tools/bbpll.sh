#!/bin/sh

./pdma-rw-reg /dev/pdma 1 0 0x00370000
./pdma-rw-reg /dev/pdma 0 0

./pdma-rw-reg /dev/pdma 1 0 0x80091700
./pdma-rw-reg /dev/pdma 1 0 0x80121000
./pdma-rw-reg /dev/pdma 1 0 0x800a1000
./pdma-rw-reg /dev/pdma 1 0 0x803a3c00
sleep 1s
./pdma-rw-reg /dev/pdma 1 0 0x80450000
./pdma-rw-reg /dev/pdma 1 0 0x82ab0600
./pdma-rw-reg /dev/pdma 1 0 0x82ac7300
./pdma-rw-reg /dev/pdma 1 0 0x80460600
./pdma-rw-reg /dev/pdma 1 0 0x8048e800 ##
./pdma-rw-reg /dev/pdma 1 0 0x80495b00 ##
./pdma-rw-reg /dev/pdma 1 0 0x804a3500 ##
./pdma-rw-reg /dev/pdma 1 0 0x804be000
./pdma-rw-reg /dev/pdma 1 0 0x804e1000
./pdma-rw-reg /dev/pdma 1 0 0x80437100 ##
./pdma-rw-reg /dev/pdma 1 0 0x8042cf00 ##
./pdma-rw-reg /dev/pdma 1 0 0x80411500 ##
./pdma-rw-reg /dev/pdma 1 0 0x80440e00 ##
./pdma-rw-reg /dev/pdma 1 0 0x803f0500
sleep 1s
./pdma-rw-reg /dev/pdma 1 0 0x803f0100
./pdma-rw-reg /dev/pdma 1 0 0x804c8600
./pdma-rw-reg /dev/pdma 1 0 0x804d0100
./pdma-rw-reg /dev/pdma 1 0 0x804d0500
sleep 1s
./pdma-rw-reg /dev/pdma 1 0 0x005e0000
./pdma-rw-reg /dev/pdma 0 0

