#include <stdio.h>
#include "json-server.h"

//Returns the total size of the virtual address space for the running process
static long get_memory_usage_linux()
{
	//Variables to store all the contents of the stat file
	int pid, ppid, pgrp, session, tty_nr, tpgid;
	char comm[2048], state;
	unsigned int flags;
	unsigned long minflt, cminflt, majflt, cmajflt, vsize;
	unsigned long utime, stime;
	long cutime, cstime, priority, nice, num_threads, itrealvalue, rss;
	unsigned long long starttime;
	// Open the file
	FILE *stat = fopen("/proc/self/stat", "r");
	if (!stat) {
		perror("Failed to open /proc/self/stat");
		return 0;
	}
	// Read the statistics out of the file
	fscanf(stat, 	"%d%s%c%d%d%d%d%d%u%lu%lu%lu%lu"
			"%ld%ld%ld%ld%ld%ld%ld%ld%llu%lu%ld",
			&pid, comm, &state, &ppid, &pgrp, &session, &tty_nr,
			&tpgid, &flags, &minflt, &cminflt, &majflt, &cmajflt,
			&utime, &stime, &cutime, &cstime, &priority, &nice,
			&num_threads, &itrealvalue, &starttime, &vsize, &rss);
	fclose(stat);
	return vsize;
}
