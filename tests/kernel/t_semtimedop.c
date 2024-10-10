/*	$NetBSD: t_semtimedop.c,v 1.2 2024/10/10 07:45:02 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_semtimedop.c,v 1.2 2024/10/10 07:45:02 martin Exp $");

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <atf-c.h>
#include <sys/wait.h>

union semun {
	int	val;		/* value for SETVAL */
	struct	semid_ds *buf;	/* buffer for IPC_{STAT,SET} */
	u_short	*array;		/* array for GETALL & SETALL */
};

ATF_TC(semtimedop_basic);
ATF_TC_HEAD(semtimedop_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "Basic semtimedop functionality");
}

ATF_TC_BODY(semtimedop_basic, tc)
{
	key_t		key = IPC_PRIVATE;
	int		semid;
	struct sembuf	sops;
	union semun	sun;
	struct timespec	timeout;

	/* Create semaphore set with 1 semaphore */
	semid = semget(key, 1, IPC_CREAT | 0600);
	ATF_REQUIRE_MSG(semid != -1, "semget failed: %s", strerror(errno));

	/* Initialize semaphore to 0 */
	sun.val = 0;
	if (semctl(semid, 0, SETVAL, sun) == -1) {
		ATF_REQUIRE_MSG(0, "semctl SETVAL failed: %s",
		    strerror(errno));
	}

	/* Define semtimedop operation: increment semaphore */
	sops.sem_num = 0;
	sops.sem_op = 1;
	sops.sem_flg = 0;

	/* Define timeout */
	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;

	/* Perform semtimedop */
	if (semtimedop(semid, &sops, 1, &timeout) == -1) {
		ATF_REQUIRE_MSG(0, "semtimedop failed: %s", strerror(errno));
	}

	/* Check semaphore value */
	int val = semctl(semid, 0, GETVAL);
	ATF_REQUIRE_MSG(val == 1,
	    "Semaphore value incorrect: got %d, expected 1", val);

	/* Clean up */
	ATF_REQUIRE_MSG(semctl(semid, 0, IPC_RMID) != -1,
	    "semctl IPC_RMID failed: %s", strerror(errno));
}

/* semtimedop blocks until timeout expires */
ATF_TC(semtimedop_timeout);
ATF_TC_HEAD(semtimedop_timeout, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "semtimedop blocks until timeout expires");
}

ATF_TC_BODY(semtimedop_timeout, tc)
{
	key_t		key = IPC_PRIVATE;
	int		semid;
	struct sembuf	sops;
	union semun	sun;
	struct timespec	timeout;
	pid_t		pid;
	int		status;

	/* Create semaphore set with 1 semaphore */
	semid = semget(key, 1, IPC_CREAT | 0600);
	ATF_REQUIRE_MSG(semid != -1, "semget failed: %s", strerror(errno));

	/* Initialize semaphore to 0 */
	sun.val = 0;
	if (semctl(semid, 0, SETVAL, sun) == -1) {
		ATF_REQUIRE_MSG(0, "semctl SETVAL failed: %s", strerror(errno));
	}

	pid = fork();
	ATF_REQUIRE_MSG(pid != -1, "fork failed: %s", strerror(errno));

	if (pid == 0) {
		/*
		 * Child: perform semtimedop with negative sem_op, should
		 * block until timeout
		 */
		sops.sem_num = 0;
		sops.sem_op = -1;
		sops.sem_flg = 0;

		timeout.tv_sec = 2;
		timeout.tv_nsec = 0;

		if (semtimedop(semid, &sops, 1, &timeout) == -1) {
			if (errno == EAGAIN || errno == EINTR) {
				exit(0);	/* Expected */
			}
		}
		exit(1);	/* Unexpected failure/success */
	}

	/* Parent: wait for child to finish */
	waitpid(pid, &status, 0);
	ATF_REQUIRE(WIFEXITED(status));
	ATF_REQUIRE_EQ(WEXITSTATUS(status), 0);

	/* Clean up */
	ATF_REQUIRE_MSG(semctl(semid, 0, IPC_RMID) != -1,
	    "semctl IPC_RMID failed: %s", strerror(errno));
}

/* semtimedop with SEM_UNDO adjusts semaphore on exit */
ATF_TC(semtimedop_semundo);
ATF_TC_HEAD(semtimedop_semundo, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "semtimedop with SEM_UNDO adjusts semaphore on exit");
}

ATF_TC_BODY(semtimedop_semundo, tc)
{
	key_t		key = IPC_PRIVATE;
	int		semid;
	struct sembuf	sops;
	union semun	sun;
	struct timespec	timeout;
	pid_t		pid;
	int		val;

	/* Create semaphore set with 1 semaphore */
	semid = semget(key, 1, IPC_CREAT | 0600);
	ATF_REQUIRE_MSG(semid != -1, "semget failed: %s", strerror(errno));

	/* Initialize semaphore to 0 */
	sun.val = 0;
	if (semctl(semid, 0, SETVAL, sun) == -1) {
		ATF_REQUIRE_MSG(0, "semctl SETVAL failed: %s", strerror(errno));
	}

	pid = fork();
	ATF_REQUIRE_MSG(pid != -1, "fork failed: %s", strerror(errno));

	if (pid == 0) {
		/* Child: perform semtimedop with SEM_UNDO */
		sops.sem_num = 0;
		sops.sem_op = 1;
		sops.sem_flg = SEM_UNDO;

		timeout.tv_sec = 1;
		timeout.tv_nsec = 0;

		if (semtimedop(semid, &sops, 1, &timeout) == -1) {
			exit(1);	/* Unexpected failure */
		}

		exit(0); /* Exit normally, SEM_UNDO should be triggered */
	}

	/* Parent: wait for child to exit */
	waitpid(pid, NULL, 0);

	/* Check semaphore value; should be 0 after SEM_UNDO */
	val = semctl(semid, 0, GETVAL);
	ATF_REQUIRE_MSG(val == 0,
	    "Semaphore value incorrect after SEM_UNDO: got %d, "
	    "expected 0", val);

	/* Clean up */
	ATF_REQUIRE_MSG(semctl(semid, 0, IPC_RMID) != -1,
	    "semctl IPC_RMID failed: %s", strerror(errno));
}

/* semtimedop handles invalid parameters correctly */
ATF_TC(semtimedop_invalid);
ATF_TC_HEAD(semtimedop_invalid, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "semtimedop handles invalid parameters correctly");
}

ATF_TC_BODY(semtimedop_invalid, tc)
{
	struct sembuf	sops;
	union semun	sun;
	struct timespec	timeout;

	/* Invalid semaphore id */
	sops.sem_num = 0;
	sops.sem_op = -1;
	sops.sem_flg = 0;

	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;

	/* Attempt to perform semtimedop on invalid semid */
	ATF_REQUIRE_MSG(semtimedop(-1, &sops, 1, &timeout) == -1
	    && errno == EINVAL, "semtimedop did not fail on invalid semid");

	/* Create semaphore set */
	key_t	key = IPC_PRIVATE;
	int	semid = semget(key, 1, IPC_CREAT | 0600);
	ATF_REQUIRE_MSG(semid != -1, "semget failed: %s", strerror(errno));

	/* Initialize semaphore to 0 */
	sun.val = 0;
	if (semctl(semid, 0, SETVAL, sun) == -1) {
		ATF_REQUIRE_MSG(0, "semctl SETVAL failed: %s", strerror(errno));
	}

	/* Set an invalid semaphore number */
	sops.sem_num = 1;	/* Only 1 semaphore in set, index 0 */
	sops.sem_op = 1;
	sops.sem_flg = 0;

	/* Attempt semtimedop with invalid sem_num */
	ATF_REQUIRE_MSG(semtimedop(semid, &sops, 1, &timeout) == -1
	    && errno == EFBIG, "semtimedop did not fail on invalid sem_num");

	/* Clean up */
	ATF_REQUIRE_MSG(semctl(semid, 0, IPC_RMID) != -1,
	    "semctl IPC_RMID failed: %s", strerror(errno));
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, semtimedop_basic);
	ATF_TP_ADD_TC(tp, semtimedop_timeout);
	ATF_TP_ADD_TC(tp, semtimedop_semundo);
	ATF_TP_ADD_TC(tp, semtimedop_invalid);

	return atf_no_error();
}
