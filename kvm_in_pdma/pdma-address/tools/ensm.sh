./pdma-rw-reg /dev/pdma 1 0 0x80130100
./pdma-rw-reg /dev/pdma 1 0 0x80140700
./pdma-rw-reg /dev/pdma 1 0 0x80142300
./pdma-rw-reg /dev/pdma 1 0 0x80150400
./pdma-rw-reg /dev/pdma 1 0 0x8016f300
sleep 1s
./pdma-rw-reg /dev/pdma 1 0 0x00170000
sleep 1s
./pdma-rw-reg /dev/pdma 0 0
