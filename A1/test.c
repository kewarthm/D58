#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if(fork() == 0) {
	printf("child\n");
    } else {
	printf("parent\n");
    }
}
