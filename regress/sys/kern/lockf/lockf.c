/*	$NetBSD: lockf.c,v 1.5 2002/02/21 07:38:20 itojun Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * lockf regression test:
 *
 * Tests:
 * 1) fork N child processes, do a bunch of random byte range lock/unlock.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h> 

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <signal.h>
#include <errno.h>

int nlocks = 1000;		/* number of locks per thread */
int nprocs = 10;		/* number of processes to spawn */
int sleeptime = 500000;		/* sleep time between locks, usec */
off_t size = 8192;		/* size of file to lock */
const char *lockfile = "/tmp/lockf_test";

static u_int32_t
random_uint32(void)
{
	return lrand48();
}


static void
trylocks(int id)
{
	int i, ret, fd;
	const char *which;
	
	srand48(getpid());

	fd = open (lockfile, O_RDWR, 0);
	
	if (fd < 0)
		err(1, lockfile);
	
	printf("%d: start\n", id);
	
	for (i=0; i<nlocks; i++) {
		struct flock fl;

		fl.l_start = random_uint32() % size;
		fl.l_len = random_uint32() % size;
		switch (random_uint32() % 3) {
		case 0:
			which = "read";
			fl.l_type = F_RDLCK;
			break;
		case 1:
			which = "write";
			fl.l_type = F_WRLCK;
			break;
		case 2:
			which = "un";
			fl.l_type = F_UNLCK;
			break;
		}
		fl.l_whence = SEEK_SET;

		printf("%d: try %slock %d to %d\n", id, which, (int)fl.l_start,
		    (int)(fl.l_start + fl.l_len));
		
		ret = fcntl(fd, F_SETLKW, &fl);

		if (ret < 0)
			perror("fcntl");
		printf("%d: got %slock %d to %d\n", id, which, (int)fl.l_start,
		    ((int)(fl.l_start + fl.l_len)));
		
		if (usleep(sleeptime) < 0) 
		  err(1, "usleep");
	}
	printf("%d: done\n", id);
	close (fd);
}

/* ARGSUSED */
int
main(int argc, char **argv)
{
	int i, j;
	pid_t *pid;
	int status;
	int fd;
	
	(void)unlink(lockfile);

	fd = open (lockfile, O_RDWR|O_CREAT|O_EXCL|O_TRUNC, 0666);
	if (fd < 0)
		err(1, "%s", lockfile);

	if (ftruncate(fd, size) < 0)
		err(1, "ftruncate of %s failed", lockfile);

	fsync(fd);
	close(fd);
	
	pid = malloc(nprocs * sizeof(pid_t));
	
	for (i=0; i<nprocs; i++) {
		pid[i] = fork();
		switch (pid[i]) {
		case 0:
			trylocks(i);
			_exit(0);
			break;
		case -1:
			err(1, "fork failed");
			break;
		default:
			break;
		}
	}
	for (j=0; j<100; j++) {
		printf("parent: run %i\n", j+1);
		for (i=0; i<nprocs; i++) {
			printf("stop %d\n", i);
			if (ptrace(PT_ATTACH, pid[i], 0, 0) < 0)
				err(1, "ptrace attach %d", pid[i]);
			printf("wait %d\n", i);
			if (waitpid(pid[i], &status, WUNTRACED) < 0)
				err(1, "waitpid(ptrace)");
			printf("awake %d\n", i);
			usleep(sleeptime/3);
			if (ptrace(PT_DETACH, pid[i], (caddr_t)1, 0) < 0)
				err(1, "ptrace detach %d", pid[i]);
			printf("done %d\n", i);
			usleep(sleeptime/3);
		}
	}
	for (i=0; i<nprocs; i++) {
		printf("reap %d: ", i);
		fflush(stdout);
		kill(pid[i], SIGINT);
		waitpid(pid[i], &status, 0);
		printf(" status %d\n", status);
	}
	exit(0);
	/* NOTREACHED */
}
