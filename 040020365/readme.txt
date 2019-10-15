./make
insmod vigenere.o vigenere_major=254 kbuffersize=4 key="LINUX"
mknod /dev/vigenere c 254 0
gcc -o t 040020365_test.c
./t
