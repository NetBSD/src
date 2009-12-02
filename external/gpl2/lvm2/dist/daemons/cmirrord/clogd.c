/*	$NetBSD: clogd.c,v 1.1.1.1 2009/12/02 00:27:06 haad Exp $	*/

/*
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "configure.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/dm-ioctl.h>

#include "dm-log-userspace.h"
#include "functions.h"
#include "local.h"
#include "cluster.h"
#include "common.h"
#include "logging.h"
#include "link_mon.h"

static int exit_now = 0;
static sigset_t signal_mask;
static int signal_received;

static void process_signals(void);
static void daemonize(void);
static void init_all(void);
static void cleanup_all(void);

int main(int argc, char *argv[])
{
	daemonize();

	init_all();

	/* Parent can now exit, we're ready to handle requests */
	kill(getppid(), SIGTERM);

	LOG_PRINT("Starting cmirrord:");
	LOG_PRINT(" Built: "__DATE__" "__TIME__"\n");
	LOG_DBG(" Compiled with debugging.");

	while (!exit_now) {
		links_monitor();

		links_issue_callbacks();

		process_signals();
	}
	exit(EXIT_SUCCESS);
}

/*
 * parent_exit_handler: exit the parent
 * @sig: the signal
 *
 */
static void parent_exit_handler(int sig)
{
	exit_now = 1;
}

/*
 * create_lockfile - create and lock a lock file
 * @lockfile: location of lock file
 *
 * Returns: 0 on success, -1 otherwise
 */
static int create_lockfile(char *lockfile)
{
	int fd;
	struct flock lock;
	char buffer[50];

	if((fd = open(lockfile, O_CREAT | O_WRONLY,
		      (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))) < 0)
		return -errno;

	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	if (fcntl(fd, F_SETLK, &lock) < 0) {
		close(fd);
		return -errno;
	}

	if (ftruncate(fd, 0) < 0) {
		close(fd);
		return -errno;
	}

	sprintf(buffer, "%d\n", getpid());

	if(write(fd, buffer, strlen(buffer)) < strlen(buffer)){
		close(fd);
		unlink(lockfile);
		return -errno;
	}

	return 0;
}

static void sig_handler(int sig)
{
	sigaddset(&signal_mask, sig);
	++signal_received;
}

static void process_signal(int sig){
	int r = 0;

	switch(sig) {
	case SIGINT:
	case SIGQUIT:
	case SIGTERM:
	case SIGHUP:
		r += log_status();
		break;
	case SIGUSR1:
	case SIGUSR2:
		log_debug();
		/*local_debug();*/
		cluster_debug();
		return;
	default:
		LOG_PRINT("Unknown signal received... ignoring");
		return;
	}

	if (!r) {
		LOG_DBG("No current cluster logs... safe to exit.");
		cleanup_all();
		exit(EXIT_SUCCESS);
	}

	LOG_ERROR("Cluster logs exist.  Refusing to exit.");
}

static void process_signals(void)
{
	int x;

	if (!signal_received)
		return;

	signal_received = 0;

	for (x = 1; x < _NSIG; x++) {
		if (sigismember(&signal_mask, x)) {
			sigdelset(&signal_mask, x);
			process_signal(x);
		}
	}
}

/*
 * daemonize
 *
 * Performs the steps necessary to become a daemon.
 */
static void daemonize(void)
{
	int pid;
	int status;

	signal(SIGTERM, &parent_exit_handler);

	pid = fork();

	if (pid < 0) {
		LOG_ERROR("Unable to fork()");
		exit(EXIT_FAILURE);
	}

	if (pid) {
		/* Parent waits here for child to get going */
		while (!waitpid(pid, &status, WNOHANG) && !exit_now);
		if (exit_now)
			exit(EXIT_SUCCESS);

		switch (WEXITSTATUS(status)) {
		case EXIT_LOCKFILE:
			LOG_ERROR("Failed to create lockfile");
			LOG_ERROR("Process already running?");
			break;
		case EXIT_KERNEL_SOCKET:
			LOG_ERROR("Unable to create netlink socket");
			break;
		case EXIT_KERNEL_BIND:
			LOG_ERROR("Unable to bind to netlink socket");
			break;
		case EXIT_KERNEL_SETSOCKOPT:
			LOG_ERROR("Unable to setsockopt on netlink socket");
			break;
		case EXIT_CLUSTER_CKPT_INIT:
			LOG_ERROR("Unable to initialize checkpoint service");
			LOG_ERROR("Has the cluster infrastructure been started?");
			break;
		case EXIT_FAILURE:
			LOG_ERROR("Failed to start: Generic error");
			break;
		default:
			LOG_ERROR("Failed to start: Unknown error");
			break;
		}
		exit(EXIT_FAILURE);
	}

	setsid();
	chdir("/");
	umask(0);

	close(0); close(1); close(2);
	open("/dev/null", O_RDONLY); /* reopen stdin */
	open("/dev/null", O_WRONLY); /* reopen stdout */
	open("/dev/null", O_WRONLY); /* reopen stderr */

	LOG_OPEN("cmirrord", LOG_PID, LOG_DAEMON);

	if (create_lockfile(CMIRRORD_PIDFILE))
		exit(EXIT_LOCKFILE);

	signal(SIGINT, &sig_handler);
	signal(SIGQUIT, &sig_handler);
	signal(SIGTERM, &sig_handler);
	signal(SIGHUP, &sig_handler);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1, &sig_handler);
	signal(SIGUSR2, &sig_handler);
	sigemptyset(&signal_mask);
	signal_received = 0;
}

/*
 * init_all
 *
 * Initialize modules.  Exit on failure.
 */
static void init_all(void)
{
	int r;

	if ((r = init_local()) ||
	    (r = init_cluster())) {
		exit(r);
	}
}

/*
 * cleanup_all
 *
 * Clean up before exiting
 */
static void cleanup_all(void)
{
	cleanup_local();
	cleanup_cluster();
}
