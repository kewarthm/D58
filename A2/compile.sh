gcc -c -g common.c
gcc -c -g packet.c
gcc -c -g client.c
gcc -c -g server.c
gcc -c -g end_point.c
gcc -c -g router.c
gcc common.o client.o end_point.o packet.o -o end_point -lpthread
gcc common.o server.o router.o packet.o -o router -lpthread
