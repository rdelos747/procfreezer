// /////////////////
// I N I T
// ///////////////////////////
#include <stdio.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <locale.h>

#define MAX_COUNTERS 256
static unsigned int NUM_CPUS = 0;
static int fd_cache[MAX_COUNTERS];
static int fd_itlb[MAX_COUNTERS];

long getLong(char* c) {
	char *p;
	return strtol(c, &p, 10);
}

// /////////////////
// P E R F
// ///////////////////////////
long sys_perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
	int ret;
	ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
	return ret;
}

long long run_perf(int* fd) {
	long long eventContents = 0;
	long long totalEvents = 0;
	unsigned int counter;
	for (counter = 0; counter < NUM_CPUS; counter++) {
		if (fd[counter] != -1) {
			read(fd[counter], &eventContents, sizeof(long long));
			totalEvents += eventContents;
		} else {
			fprintf(stderr, "Failed to read cpu %d\n", counter);
		}
		ioctl(fd[counter], PERF_EVENT_IOC_RESET, 0);
	}
	return totalEvents;
}

// /////////////////
// M A I N
// ///////////////////////////
int main(int argc, char **argv) {

	long times = 10;
	long refresh = 100000000;
	if (argc > 1) {
		times = getLong(argv[1]);
	}

	if (argc > 2) {
		refresh = getLong(argv[2]);
	}

	NUM_CPUS = sysconf(_SC_NPROCESSORS_ONLN);
	struct timespec time, time2;
	time.tv_sec = 0;
	time.tv_nsec = refresh;
	if (refresh - 1000000000 >= 0) {
		printf("Max time = 1s\n");
		time.tv_sec = 1;
		time.tv_nsec = 0;
	}

	// CACHE MISSES
	struct perf_event_attr cache_attr;
	memset(&cache_attr, 0, sizeof(struct perf_event_attr));
	cache_attr.type = PERF_TYPE_HARDWARE;
	cache_attr.config = PERF_COUNT_HW_CACHE_MISSES;
	cache_attr.size = sizeof(struct perf_event_attr);
	cache_attr.disabled = 0;

	// ITLB
	struct perf_event_attr itlb_attr;
	memset(&itlb_attr, 0, sizeof(struct perf_event_attr));
	itlb_attr.type = PERF_TYPE_HW_CACHE;
	itlb_attr.config = ((PERF_COUNT_HW_CACHE_ITLB) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
	itlb_attr.size = sizeof(struct perf_event_attr);
	itlb_attr.disabled = 0;

	unsigned int cpu = 0;
	for (cpu = 0; cpu < NUM_CPUS; cpu++) {
		fd_cache[cpu] = sys_perf_event_open(&cache_attr, -1, cpu, -1, 0);
	}

	for (cpu = 0; cpu < NUM_CPUS; cpu++) {
		fd_itlb[cpu] = sys_perf_event_open(&itlb_attr, -1, cpu, -1, 0);
	}

	long long cache_num;
	long long itlb_num;
	printf("Running %ld times\n", times);
	printf("at %ld times per second\n\n", 1000000000 / refresh);
	long ii = 0;
	float ratio = 0;
	while (ii < times) {
		nanosleep(&time, &time2);
		cache_num = run_perf(fd_cache);
		itlb_num = run_perf(fd_itlb);
		setlocale(LC_NUMERIC, "");
		printf("Cache misses = %'10lld\n", cache_num);
		printf("        Itlb = %'10lld\n", itlb_num);
		float f = (float)cache_num / itlb_num;
		if (f > ratio) {
			ratio = f;
		}
		printf("Ratio: %.6f\n\n", f);
		ii++;
	}

	printf("Highest Ratio Found: %.6f\n", ratio);
	return 1;
}
