/*
 * Copyright (C) 2003 Sistina Software
 *
 * This file is released under the LGPL.
 */

#include "dm.h"
#include "dm-daemon.h"

#include <linux/module.h>
#include <linux/sched.h>

static int daemon(void *arg)
{
	struct dm_daemon *dd = (struct dm_daemon *) arg;
	DECLARE_WAITQUEUE(wq, current);

	daemonize();
	reparent_to_init();

	/* block all signals */
	spin_lock_irq(&current->sigmask_lock);
	sigfillset(&current->blocked);
	flush_signals(current);
	spin_unlock_irq(&current->sigmask_lock);

	strcpy(current->comm, dd->name);
	atomic_set(&dd->please_die, 0);

	add_wait_queue(&dd->job_queue, &wq);

	down(&dd->run_lock);
	up(&dd->start_lock);

	/*
	 * dd->fn() could do anything, very likely it will
	 * suspend.  So we can't set the state to
	 * TASK_INTERRUPTIBLE before calling it.  In order to
	 * prevent a race with a waking thread we do this little
	 * dance with the dd->woken variable.
	 */
	while (1) {
		do {
			set_current_state(TASK_RUNNING);

			if (atomic_read(&dd->please_die))
				goto out;

			atomic_set(&dd->woken, 0);
			dd->fn();
			yield();

			set_current_state(TASK_INTERRUPTIBLE);
		} while (atomic_read(&dd->woken));

		schedule();
	}

 out:
	remove_wait_queue(&dd->job_queue, &wq);
	up(&dd->run_lock);
	return 0;
}

int dm_daemon_start(struct dm_daemon *dd, const char *name, void (*fn)(void))
{
	pid_t pid = 0;

	/*
	 * Initialise the dm_daemon.
	 */
	dd->fn = fn;
	strncpy(dd->name, name, sizeof(dd->name) - 1);
	sema_init(&dd->start_lock, 1);
	sema_init(&dd->run_lock, 1);
	init_waitqueue_head(&dd->job_queue);

	/*
	 * Start the new thread.
	 */
	down(&dd->start_lock);
	pid = kernel_thread(daemon, dd, 0);
	if (pid <= 0) {
		DMERR("Failed to start %s thread", name);
		return -EAGAIN;
	}

	/*
	 * wait for the daemon to up this mutex.
	 */
	down(&dd->start_lock);
	up(&dd->start_lock);

	return 0;
}

void dm_daemon_stop(struct dm_daemon *dd)
{
	atomic_set(&dd->please_die, 1);
	dm_daemon_wake(dd);
	down(&dd->run_lock);
	up(&dd->run_lock);
}

void dm_daemon_wake(struct dm_daemon *dd)
{
	atomic_set(&dd->woken, 1);
	wake_up_interruptible(&dd->job_queue);
}

EXPORT_SYMBOL(dm_daemon_start);
EXPORT_SYMBOL(dm_daemon_stop);
EXPORT_SYMBOL(dm_daemon_wake);
