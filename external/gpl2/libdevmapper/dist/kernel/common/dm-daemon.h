/*
 * Copyright (C) 2003 Sistina Software
 *
 * This file is released under the LGPL.
 */

#ifndef DM_DAEMON_H
#define DM_DAEMON_H

#include <asm/atomic.h>
#include <asm/semaphore.h>

struct dm_daemon {
	void (*fn)(void);
	char name[16];
	atomic_t please_die;
	struct semaphore start_lock;
	struct semaphore run_lock;

	atomic_t woken;
	wait_queue_head_t job_queue;
};

int dm_daemon_start(struct dm_daemon *dd, const char *name, void (*fn)(void));
void dm_daemon_stop(struct dm_daemon *dd);
void dm_daemon_wake(struct dm_daemon *dd);
int dm_daemon_running(struct dm_daemon *dd);

#endif
