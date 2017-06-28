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

static int fd_cpu[MAX_COUNTERS];
static int fd_bus[MAX_COUNTERS];
static int fd_instr[MAX_COUNTERS];
static int fd_bi[MAX_COUNTERS];
static int fd_bm[MAX_COUNTERS];
static int fd_bpuA[MAX_COUNTERS];
static int fd_bpuM[MAX_COUNTERS];
static int fd_cacheR[MAX_COUNTERS];
static int fd_cache[MAX_COUNTERS];
static int fd_dtlbA[MAX_COUNTERS];
static int fd_dtlbM[MAX_COUNTERS];
static int fd_itlbA[MAX_COUNTERS];
static int fd_itlbM[MAX_COUNTERS];

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

	// FILE
	char *file_name = argv[1];
	FILE *f = fopen(file_name, "w");

	NUM_CPUS = sysconf(_SC_NPROCESSORS_ONLN);

	// CPU CYCLES
	struct perf_event_attr cpu_attr;
	memset(&cpu_attr, 0, sizeof(struct perf_event_attr));
	cpu_attr.type = PERF_TYPE_HARDWARE;
	cpu_attr.config = PERF_COUNT_HW_CPU_CYCLES;
	cpu_attr.size = sizeof(struct perf_event_attr);
	cpu_attr.disabled = 0;

	// BUS CYCLES
	struct perf_event_attr bus_attr;
	memset(&bus_attr, 0, sizeof(struct perf_event_attr));
	bus_attr.type = PERF_TYPE_HARDWARE;
	bus_attr.config = PERF_COUNT_HW_BUS_CYCLES;
	bus_attr.size = sizeof(struct perf_event_attr);
	bus_attr.disabled = 0;

	// INSTRUCTIONS
	struct perf_event_attr instr_attr;
	memset(&instr_attr, 0, sizeof(struct perf_event_attr));
	instr_attr.type = PERF_TYPE_HARDWARE;
	instr_attr.config = PERF_COUNT_HW_INSTRUCTIONS;
	instr_attr.size = sizeof(struct perf_event_attr);
	instr_attr.disabled = 0;

	// BRANCH INSTRUCTIONS
	struct perf_event_attr bi_attr;
	memset(&bi_attr, 0, sizeof(struct perf_event_attr));
	bi_attr.type = PERF_TYPE_HARDWARE;
	bi_attr.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
	bi_attr.size = sizeof(struct perf_event_attr);
	bi_attr.disabled = 0;

	// BRANCH MISSES
	struct perf_event_attr bm_attr;
	memset(&bm_attr, 0, sizeof(struct perf_event_attr));
	bm_attr.type = PERF_TYPE_HARDWARE;
	bm_attr.config = PERF_COUNT_HW_BRANCH_MISSES;
	bm_attr.size = sizeof(struct perf_event_attr);
	bm_attr.disabled = 0;

	// BPU ACCESS
	struct perf_event_attr bpuA_attr;
	memset(&bpuA_attr, 0, sizeof(struct perf_event_attr));
	bpuA_attr.type = PERF_TYPE_HW_CACHE;
	bpuA_attr.config = ((PERF_COUNT_HW_CACHE_BPU) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16));
	bpuA_attr.size = sizeof(struct perf_event_attr);
	bpuA_attr.disabled = 0;

	// BPU MISS
	struct perf_event_attr bpuM_attr;
	memset(&bpuM_attr, 0, sizeof(struct perf_event_attr));
	bpuM_attr.type = PERF_TYPE_HW_CACHE;
	bpuM_attr.config = ((PERF_COUNT_HW_CACHE_BPU) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
	bpuM_attr.size = sizeof(struct perf_event_attr);
	bpuM_attr.disabled = 0;

	// CACHE REFERENCES
	struct perf_event_attr cacheR_attr;
	memset(&cacheR_attr, 0, sizeof(struct perf_event_attr));
	cacheR_attr.type = PERF_TYPE_HARDWARE;
	cacheR_attr.config = PERF_COUNT_HW_CACHE_REFERENCES;
	cacheR_attr.size = sizeof(struct perf_event_attr);
	cacheR_attr.disabled = 0;

	// CACHE MISSES
	struct perf_event_attr cache_attr;
	memset(&cache_attr, 0, sizeof(struct perf_event_attr));
	cache_attr.type = PERF_TYPE_HARDWARE;
	cache_attr.config = PERF_COUNT_HW_CACHE_MISSES;
	cache_attr.size = sizeof(struct perf_event_attr);
	cache_attr.disabled = 0;

	// DTLB ACCESS
	struct perf_event_attr dtlbA_attr;
	memset(&dtlbA_attr, 0, sizeof(struct perf_event_attr));
	dtlbA_attr.type = PERF_TYPE_HW_CACHE;
	dtlbA_attr.config = ((PERF_COUNT_HW_CACHE_DTLB) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16));
	dtlbA_attr.size = sizeof(struct perf_event_attr);
	dtlbA_attr.disabled = 0;

	// DTLB MISS
	struct perf_event_attr dtlbM_attr;
	memset(&dtlbM_attr, 0, sizeof(struct perf_event_attr));
	dtlbM_attr.type = PERF_TYPE_HW_CACHE;
	dtlbM_attr.config = ((PERF_COUNT_HW_CACHE_DTLB) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
	dtlbM_attr.size = sizeof(struct perf_event_attr);
	dtlbM_attr.disabled = 0;

	// ITLB ACCESS
	struct perf_event_attr itlbA_attr;
	memset(&itlbA_attr, 0, sizeof(struct perf_event_attr));
	itlbA_attr.type = PERF_TYPE_HW_CACHE;
	itlbA_attr.config = ((PERF_COUNT_HW_CACHE_ITLB) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16));
	itlbA_attr.size = sizeof(struct perf_event_attr);
	itlbA_attr.disabled = 0;

	// ITLB MISS
	struct perf_event_attr itlbM_attr;
	memset(&itlbM_attr, 0, sizeof(struct perf_event_attr));
	itlbM_attr.type = PERF_TYPE_HW_CACHE;
	itlbM_attr.config = ((PERF_COUNT_HW_CACHE_ITLB) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
	itlbM_attr.size = sizeof(struct perf_event_attr);
	itlbM_attr.disabled = 0;

	unsigned int cpu = 0;
	for (cpu = 0; cpu < NUM_CPUS; cpu++) {
		fd_cpu[cpu] = sys_perf_event_open(&cpu_attr, -1, cpu, -1, 0);
		fd_bus[cpu] = sys_perf_event_open(&bus_attr, -1, cpu, -1, 0);
		fd_instr[cpu] = sys_perf_event_open(&instr_attr, -1, cpu, -1, 0);
		fd_bi[cpu] = sys_perf_event_open(&bi_attr, -1, cpu, -1, 0);
		fd_bm[cpu] = sys_perf_event_open(&bm_attr, -1, cpu, -1, 0);
		fd_bpuA[cpu] = sys_perf_event_open(&bpuA_attr, -1, cpu, -1, 0);
		fd_bpuM[cpu] = sys_perf_event_open(&bpuM_attr, -1, cpu, -1, 0);
		fd_cacheR[cpu] = sys_perf_event_open(&cacheR_attr, -1, cpu, -1, 0);
		fd_cache[cpu] = sys_perf_event_open(&cache_attr, -1, cpu, -1, 0);
		fd_dtlbA[cpu] = sys_perf_event_open(&dtlbA_attr, -1, cpu, -1, 0);
		fd_dtlbM[cpu] = sys_perf_event_open(&dtlbM_attr, -1, cpu, -1, 0);
		fd_itlbA[cpu] = sys_perf_event_open(&itlbA_attr, -1, cpu, -1, 0);
		fd_itlbM[cpu] = sys_perf_event_open(&itlbM_attr, -1, cpu, -1, 0);
	}

	long long cpu_num;
	long long bus_num;
	long long instr_num;
	long long bi_num;
	long long bm_num;
	long long bpuA_num;
	long long bpuM_num;
	long long cacheR_num;
	long long cache_num;
	long long dtlbA_num;
	long long dtlbM_num;
	long long itlbA_num;
	long long itlbM_num;

	struct timespec time, time2;
	long refresh = 1000000000;
	int times = 100;
	int results[times * 10][14];

	int n = 0;
	while (refresh > 0) {
		if (refresh == 1000000000) {
			time.tv_sec = 1;
			time.tv_nsec = 0;
		} else {
			time.tv_sec = 0;
			time.tv_nsec = refresh;
		}

		int i;
		for (i = 0; i < times; i++) {
			nanosleep(&time, &time2);

			cpu_num = run_perf(fd_cpu);
			bus_num = run_perf(fd_bus);
			instr_num = run_perf(fd_instr);
			bi_num = run_perf(fd_bi);
			bm_num = run_perf(fd_bm);
			bpuA_num = run_perf(fd_bpuA);
			bpuM_num = run_perf(fd_bpuM);
			cacheR_num = run_perf(fd_cache);
			cache_num = run_perf(fd_cache);
			dtlbA_num = run_perf(fd_itlbA);
			dtlbM_num = run_perf(fd_itlbM);
			itlbA_num = run_perf(fd_itlbA);
			itlbM_num = run_perf(fd_itlbM);

			results[n][0] = refresh;
			results[n][1] = cpu_num;
			results[n][2] = bus_num;
			results[n][3] = instr_num;
			results[n][4] = bi_num;
			results[n][5] = bm_num;
			results[n][6] = bpuA_num;
			results[n][7] = bpuM_num;
			results[n][8] = cacheR_num;
			results[n][9] = cache_num;
			results[n][10] = dtlbA_num;
			results[n][11] = dtlbM_num;
			results[n][12] = itlbA_num;
			results[n][13] = itlbM_num;
			n++;
		}

		refresh = refresh / 10;
	}
	/*
	while (ii < times) {
		nanosleep(&time, &time2);
		cache_num = run_perf(fd_cache);
		itlb_num = run_perf(fd_itlb);
		setlocale(LC_NUMERIC, "");
		//printf("Cache misses = %'10lld\n", cache_num);
		//printf("        Itlb = %'10lld\n", itlb_num);
		float f = (float)cache_num / itlb_num;
		if (f > ratio) {
			ratio = f;
		}
		//printf("Ratio: %.6f\n\n", f);
		results[ii][0] = refresh;
		results[ii][1] = times;
		results[ii][2] = cache_num;
		results[ii][3] = itlb_num;
		ii++;
	}

	//printf("Highest Ratio Found: %.6f\n", ratio);
	//printf("%.6f\n",ratio);
	*/
	int i;
	fprintf(f, "Refresh rate, cpu cycles, bus cycles, instr, branch instructions, branch misses, bpu access, bpu miss, cache ref, cache misses, dtlb read, dtlb miss, itlb read accesses, itlb read misses\n");
	for (i = 0; i < n; i++) {
		fprintf(f, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", results[i][0], results[i][1], results[i][2], results[i][3], results[i][4], results[i][5], results[i][6], results[i][7], results[i][8], results[i][9], results[i][10], results[i][11], results[i][12], results[i][13]);
	}

	fclose(f);
	return 1;
}
