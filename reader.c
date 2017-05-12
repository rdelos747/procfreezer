#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

//char addr[256] = {};
int main() {
	printf("I am: %d\n", getpid());
	int fd = open("test_file.txt", O_RDONLY);
	char* addr = (unsigned char*)mmap(0, 256, PROT_READ, MAP_SHARED, fd, 0);
	char o;
	while(1) {
		//__builtin_ia32_clflush(addr);
		char x = addr[0];
		//read(fd, x, 5);
		//printf("printing %d\n", x);
	}

	return 1;
}
