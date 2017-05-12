#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

uint64_t rdtsc() {
	uint64_t a, d;
	asm volatile ("mfence");
	asm volatile("rdtsc" : "=a" (a), "=d" (d));
	a = (d<<32) | a;
	return a;
}

void flush(void* p) {
	asm volatile ("clflush 0(%0)\n"
	:
	: "c" (p)
	: "rax");
}

int main(int argc, char** argv) {
	printf("I am: %d\n", getpid());
	int fd = open("test_file.txt", O_RDONLY);
	unsigned char* addr = (unsigned char*)mmap(0, 64*1024*1024, PROT_READ, MAP_SHARED, fd, 0);
	while(1) {
		//printf("hello00o world\n");
		//char *addr = argv[1];
		//printf("Flushing:%s\n", addr);
		size_t start = rdtsc();
		//flush(addr);
		__builtin_ia32_clflush(addr);
		size_t delta = rdtsc() - start;
		//printf("Flushed in %zu!\n", delta);
	}
	return 0;
}

