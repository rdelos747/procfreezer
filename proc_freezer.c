/*
*  - assumes /sys/fs/cgroup/freezer already exists
*  - all cgroups will be created in this directory
*/

// ///////////////////
// I N I T
// /////////////////////////////////

#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <math.h>

#define MAX_COUNTERS 256

static unsigned int NUM_CPUS = 0;
//static unsigned int counter;
static int fd_cache[MAX_COUNTERS];
static int fd_itlb[MAX_COUNTERS];

static long long BAD_NUM = 8000000;

long PROCS[1000];
long SAFE[10];
long VICTIM;
long ATTACKER;

int START;
int END;
int REAL_END;
int MID;

// ///////////////////
// P E R F
// /////////////////////////////////
long sys_perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
	int ret;
	ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
	return ret;
}

long long run_perf_stat(int* fd) {
	long long eventContents = 0;
	long long totalEvents = 0;
	int counter;
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

// ///////////////////
// P R O C S
// /////////////////////////////////
long getLong(char *c) {
	char *p;
	return strtol(c, &p, 10);
}

float getFloat(char* c) {
	return strtof(c, NULL);
}

int getStat(long id, int i) {
	int sig = kill(id, 0);
	if (sig == 0) {
		//printf("pid %ld exists ", id);

		char path[40], line[100], *p;
		snprintf(path, 40, "/proc/%ld/status", id);

		FILE* f = fopen(path, "r");

		int index = 0;
		while (fgets(line, 100, f)) {
			if (strncmp(line, "Uid:", 4) != 0)
				continue;
			p = line + 4;
			char x = p[1];
			if (x != '0') {
				//printf("adding %ld at %d\n", id, i);
				PROCS[i] = id;
				return 1;
			}
			break;
		}
		fclose(f);
	}

	return 0;
}

int getProcList() {
	DIR * proc = opendir("/proc");
	struct dirent* ent;
	long id;
	//if proc = null

	int index = 0;
	while(ent = readdir(proc)) {
		if (!isdigit(*ent->d_name))
			continue;
		id = strtol(ent->d_name, NULL, 10);
		//id = ent->d_name;
		//printf("%d\n",index);
		int c = 1;
		int i;
		for (i = 0; i < 10; i++) {
			if (id == SAFE[i]) {
				printf("Safe found at %ld not added\n", id);
				c = 0;
			}
		}
		if (c == 1) {
			index += getStat(id, index);
		}
	}

	START = 0;
	END = index - 1;
	REAL_END = END;
	printf("Added %d processes to proc list\n", END);

	closedir(proc);
}

// ///////////////////
// F R E E Z E R
// /////////////////////////////////
void moveToFreezer(int start, int end) {
	//printf("   FREEZING %d to %d\n", start, end);
	int i;
	for (i = start; i < end; i++) {
		FILE *freezer = fopen("/sys/fs/cgroup/freezer/1/tasks", "w");
		fprintf(freezer, "%d\n", (int)PROCS[i]);
		fclose(freezer);
	}
	FILE *freezer = fopen("/sys/fs/cgroup/freezer/1/freezer.state", "w");
	fprintf(freezer, "FROZEN\n");
	fclose(freezer);
	//printf("   FROZEN\n");
}
void removeFromFreezer(int start, int end) {
	FILE *freezer = fopen("/sys/fs/cgroup/freezer/1/freezer.state", "w");
	fprintf(freezer, "THAWED\n");
	fclose(freezer); 
	//printf("REMOVING %d to %d\n", start, end);
	int i;
	for (i = start; i < end; i++) {
		FILE *notFreezer = fopen("/sys/fs/cgroup/freezer/2/tasks", "w");
		fprintf(notFreezer, "%d\n", (int)PROCS[i]);
		fclose(notFreezer);
	}
	//printf("REMOVED\n");
}

// ///////////////////
// M A I N
// /////////////////////////////////
int main(int argc, char **argv) {
	// GET RATIO
	float ratio = getFloat(argv[1]);
	printf("Ratio is: %.6f\n\n", ratio);

	// GET REFRESH
	struct timespec tim, tim2;
	tim.tv_sec = 0;
	tim.tv_nsec = 100000000;
	long refresh = getLong(argv[2]);
	tim.tv_nsec = refresh;
	if (refresh - 1000000000 >= 0) {
		printf("Max time = 1s\n");
		tim.tv_sec = 1;
		tim.tv_nsec = 0;
	}
	printf("Refesh rate is %ld times per second\n\n", 1000000000 / refresh);

	int i;
	// INIT SAFE
	for (i = 0; i < 10; i++) {
		SAFE[i] = -1;
	}
	// GET SAFE PROCS
	for (i = 3; i < argc; i++) {
		SAFE[i - 3] = getLong(argv[i]);
		printf("%ld added to safe\n", SAFE[i - 3]);
	}

	// GET NUM CPUs
	NUM_CPUS = sysconf(_SC_NPROCESSORS_ONLN);

	// SET UP TIME
	//struct timespec tim, tim2;
	//tim.tv_sec = 1;
	//tim.tv_nsec = 100000000;
	//tim.tv_nsec = 0;

	// CREATE PERF EVENT (CACHE MISS)
	struct perf_event_attr cache_attr;
	memset(&cache_attr, 0, sizeof(struct perf_event_attr));
	cache_attr.type = PERF_TYPE_HARDWARE;
	cache_attr.config = PERF_COUNT_HW_CACHE_MISSES;
	cache_attr.size = sizeof(struct perf_event_attr);
	cache_attr.disabled = 0;

	// CREATE PERF EVENT (ITLB)
	struct perf_event_attr itlb_attr;
	memset(&itlb_attr, 0, sizeof(struct perf_event_attr));
	itlb_attr.type = PERF_TYPE_HW_CACHE;
	itlb_attr.config = ((PERF_COUNT_HW_CACHE_ITLB) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
	itlb_attr.size = sizeof(struct perf_event_attr);
	itlb_attr.disabled = 0;

	// ASSIGN PERF EVENT TO EACH CPU
	unsigned int cpu = 0;
	for (cpu = 0; cpu < NUM_CPUS; cpu++) {
		fd_cache[cpu] = sys_perf_event_open(&cache_attr, -1, cpu, -1, 0);
		fd_itlb[cpu] = sys_perf_event_open(&itlb_attr, -1, cpu, -1, 0);
	}

	int searching = 0;
	long long cache_num = 0;
	long long itlb_num = 0;
	float new_ratio = 0;
	//int side = 0 //0 left, 1 right
	while(1) {
		if (searching == 1) {
			removeFromFreezer(START, MID);
			printf("Unfreezing %d to %d\n", START, MID);
			if (END - START <= 1) {
				printf("found proc start %ld\n", PROCS[START]);
				printf("found proc end %ld\n", PROCS[END]);
				searching = 0;
				START = 0;
				END = REAL_END;
				removeFromFreezer(START,END);
				exit(1);
			}
			else if (new_ratio > ratio) {
				printf("	>>Misses detected between %d and %d\n", MID + 1, END);
				START = MID;
				MID = floor((END + START) / 2);
			} else {
				printf("	Nothing detected between %d and %d\n", MID + 1, END);
				END = MID;
				MID = floor((END + START) / 2);
			}
			moveToFreezer(START, MID);
			printf("Freezing %d to %d\n", START, MID);
			printf("	Looking at %d to %d\n", MID + 1, END);
		} else if (new_ratio > ratio) {
			printf("Starting Search\n");
			searching = 1;
			getProcList();
			removeFromFreezer(START, END);
			printf("Unfreezing %d to %d\n", START, END);
			MID = floor((END + START) / 2);
			moveToFreezer(START, MID);
			printf("Freezing %d to %d\n", START, MID);
			printf("	Looking at %d to %d\n", MID + 1, END);
		}

		nanosleep(&tim, &tim2);
		cache_num = run_perf_stat(fd_cache);
		itlb_num = run_perf_stat(fd_itlb);
		new_ratio = (float)cache_num / itlb_num;
		setlocale(LC_NUMERIC, "");
		//printf("me: %d\n", getpid());
		printf("Total misses = %'10lld\n", cache_num);
		printf(" Itlb misses = %'10lld\n", itlb_num);
		printf("       Ratio = %.6f\n\n", new_ratio);
	}
	return 1;
}
