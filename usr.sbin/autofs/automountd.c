/*	$NetBSD: automountd.c,v 1.2 2018/01/11 13:44:26 christos Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <kusumi.tomohiro@gmail.com>.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: automountd.c,v 1.2 2018/01/11 13:44:26 christos Exp $");

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/module.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <fs/autofs/autofs_ioctl.h>

#include "common.h"

static int nchildren = 0;
static int autofs_fd;
static int request_id;
static char nfs_def_retry[] = "1";

static void
done(int request_error, bool wildcards)
{
	struct autofs_daemon_done add;
	int error;

	memset(&add, 0, sizeof(add));
	add.add_id = request_id;
	add.add_wildcards = wildcards;
	add.add_error = request_error;

	log_debugx("completing request %d with error %d",
	    request_id, request_error);

	error = ioctl(autofs_fd, AUTOFSDONE, &add);
	if (error != 0)
		log_warn("AUTOFSDONE");
}

/*
 * Remove "fstype=whatever" from optionsp and return the "whatever" part.
 */
static char *
pick_option(const char *option, char **optionsp)
{
	char *tofree, *pair, *newoptions;
	char *picked = NULL;
	bool first = true;

	tofree = *optionsp;

	newoptions = calloc(1, strlen(*optionsp) + 1);
	if (newoptions == NULL)
		log_err(1, "calloc");

	size_t olen = strlen(option);
	while ((pair = strsep(optionsp, ",")) != NULL) {
		/*
		 * XXX: strncasecmp(3) perhaps?
		 */
		if (strncmp(pair, option, olen) == 0) {
			picked = checked_strdup(pair + olen);
		} else {
			if (first)
				first = false;
			else
				strcat(newoptions, ",");
			strcat(newoptions, pair);
		}
	}

	free(tofree);
	*optionsp = newoptions;

	return picked;
}

static void
create_subtree(const struct node *node, bool incomplete)
{
	const struct node *child;
	char *path;
	bool wildcard_found = false;

	/*
	 * Skip wildcard nodes.
	 */
	if (strcmp(node->n_key, "*") == 0)
		return;

	path = node_path(node);
	log_debugx("creating subtree at %s", path);
	create_directory(path);

	if (incomplete) {
		TAILQ_FOREACH(child, &node->n_children, n_next) {
			if (strcmp(child->n_key, "*") == 0) {
				wildcard_found = true;
				break;
			}
		}

		if (wildcard_found) {
			log_debugx("node %s contains wildcard entry; "
			    "not creating its subdirectories due to -d flag",
			    path);
			free(path);
			return;
		}
	}

	free(path);

	TAILQ_FOREACH(child, &node->n_children, n_next)
		create_subtree(child, incomplete);
}

static void
exit_callback(void)
{

	done(EIO, true);
}

__dead static void
handle_request(const struct autofs_daemon_request *adr, char *cmdline_options,
    bool incomplete_hierarchy)
{
	const char *map;
	struct node *root, *parent, *node;
	FILE *f;
	char *key, *options, *fstype, *nobrowse, *retrycnt, *tmp;
	int error;
	bool wildcards;

	log_debugx("got request %d: from %s, path %s, prefix \"%s\", "
	    "key \"%s\", options \"%s\"", adr->adr_id, adr->adr_from,
	    adr->adr_path, adr->adr_prefix, adr->adr_key, adr->adr_options);

	/*
	 * Try to notify the kernel about any problems.
	 */
	request_id = adr->adr_id;
	atexit(exit_callback);

	if (strncmp(adr->adr_from, "map ", 4) != 0) {
		log_errx(1, "invalid mountfrom \"%s\"; failing request",
		    adr->adr_from);
	}

	map = adr->adr_from + 4; /* 4 for strlen("map "); */
	root = node_new_root();
	if (adr->adr_prefix[0] == '\0' || strcmp(adr->adr_prefix, "/") == 0) {
		/*
		 * Direct map.  autofs(4) doesn't have a way to determine
		 * correct map key, but since it's a direct map, we can just
		 * use adr_path instead.
		 */
		parent = root;
		key = checked_strdup(adr->adr_path);
	} else {
		/*
		 * Indirect map.
		 */
		parent = node_new_map(root, checked_strdup(adr->adr_prefix),
		    NULL,  checked_strdup(map),
		    checked_strdup("[kernel request]"), lineno);

		if (adr->adr_key[0] == '\0')
			key = NULL;
		else
			key = checked_strdup(adr->adr_key);
	}

	/*
	 * "Wildcards" here actually means "make autofs(4) request
	 * automountd(8) action if the node being looked up does not
	 * exist, even though the parent is marked as cached".  This
	 * needs to be done for maps with wildcard entries, but also
	 * for special and executable maps.
	 */
	parse_map(parent, map, key, &wildcards);
	if (!wildcards)
		wildcards = node_has_wildcards(parent);
	if (wildcards)
		log_debugx("map may contain wildcard entries");
	else
		log_debugx("map does not contain wildcard entries");

	if (key != NULL)
		node_expand_wildcard(root, key);

	node = node_find(root, adr->adr_path);
	if (node == NULL) {
		log_errx(1, "map %s does not contain key for \"%s\"; "
		    "failing mount", map, adr->adr_path);
	}

	options = node_options(node);

	/*
	 * Append options from auto_master.
	 */
	options = concat(options, ',', adr->adr_options);

	/*
	 * Prepend options passed via automountd(8) command line.
	 */
	options = concat(cmdline_options, ',', options);

	if (node->n_location == NULL) {
		log_debugx("found node defined at %s:%d; not a mountpoint",
		    node->n_config_file, node->n_config_line);

		nobrowse = pick_option("nobrowse", &options);
		if (nobrowse != NULL && key == NULL) {
			log_debugx("skipping map %s due to \"nobrowse\" "
			    "option; exiting", map);
			done(0, true);

			/*
			 * Exit without calling exit_callback().
			 */
			quick_exit(EXIT_SUCCESS);
		}

		/*
		 * Not a mountpoint; create directories in the autofs mount
		 * and complete the request.
		 */
		create_subtree(node, incomplete_hierarchy);

		if (incomplete_hierarchy && key != NULL) {
			/*
			 * We still need to create the single subdirectory
			 * user is trying to access.
			 */
			tmp = concat(adr->adr_path, '/', key);
			node = node_find(root, tmp);
			if (node != NULL)
				create_subtree(node, false);
		}

		log_debugx("nothing to mount; exiting");
		done(0, wildcards);

		/*
		 * Exit without calling exit_callback().
		 */
		quick_exit(EXIT_SUCCESS);
	}

	log_debugx("found node defined at %s:%d; it is a mountpoint",
	    node->n_config_file, node->n_config_line);

	if (key != NULL)
		node_expand_ampersand(node, key);
	error = node_expand_defined(node);
	if (error != 0) {
		log_errx(1, "variable expansion failed for %s; "
		    "failing mount", adr->adr_path);
	}

	/*
	 * Append "automounted".
	 */
	options = concat(options, ',', "automounted");

	/*
	 * Remove "nobrowse", mount(8) doesn't understand it.
	 */
	pick_option("nobrowse", &options);

	/*
	 * Figure out fstype.
	 */
	fstype = pick_option("fstype=", &options);
	if (fstype == NULL) {
		log_debugx("fstype not specified in options; "
		    "defaulting to \"nfs\"");
		fstype = checked_strdup("nfs");
	}

	retrycnt = NULL;
	if (strcmp(fstype, "nfs") == 0) {
		/*
		 * The mount_nfs(8) command defaults to retry DEF_RETRY(10000).
		 * We do not want that behaviour, because it leaves mount_nfs(8)
		 * instances and automountd(8) children hanging forever.
		 * Disable retries unless the option was passed explicitly.
		 */
		retrycnt = pick_option("retrycnt=", &options);
		if (retrycnt == NULL) {
			log_debugx("retrycnt not specified in options; "
			    "defaulting to 1");
			retrycnt = nfs_def_retry;
		}
	}

	/*
	 * NetBSD doesn't have -o retrycnt=... option which is available
	 * on FreeBSD and DragonFlyBSD, so use -R if the target type is NFS
	 * (or add -o retrycnt=... to mount_nfs(8)).
	 */
	if (retrycnt) {
		assert(!strcmp(fstype, "nfs"));
		f = auto_popen("mount_nfs", "-o", options, "-R", retrycnt,
		    node->n_location, adr->adr_path, NULL);
	} else {
		f = auto_popen("mount", "-t", fstype, "-o", options,
		    node->n_location, adr->adr_path, NULL);
	}
	assert(f != NULL);
	error = auto_pclose(f);
	if (error != 0)
		log_errx(1, "mount failed");

	log_debugx("mount done; exiting");
	done(0, wildcards);

	/*
	 * Exit without calling exit_callback().
	 */
	quick_exit(EXIT_SUCCESS);
}

static void
sigchld_handler(int dummy __unused)
{

	/*
	 * The only purpose of this handler is to make SIGCHLD
	 * interrupt the AUTOFSREQUEST ioctl(2), so we can call
	 * wait_for_children().
	 */
}

static void
register_sigchld(void)
{
	struct sigaction sa;
	int error;

	bzero(&sa, sizeof(sa));
	sa.sa_handler = sigchld_handler;
	sigfillset(&sa.sa_mask);
	error = sigaction(SIGCHLD, &sa, NULL);
	if (error != 0)
		log_err(1, "sigaction");
}


static int
wait_for_children(bool block)
{
	pid_t pid;
	int status;
	int num = 0;

	for (;;) {
		/*
		 * If "block" is true, wait for at least one process.
		 */
		if (block && num == 0)
			pid = wait4(-1, &status, 0, NULL);
		else
			pid = wait4(-1, &status, WNOHANG, NULL);
		if (pid <= 0)
			break;
		if (WIFSIGNALED(status)) {
			log_warnx("child process %d terminated with signal %d",
			    pid, WTERMSIG(status));
		} else if (WEXITSTATUS(status) != 0) {
			log_debugx("child process %d terminated with exit "
			    "status %d", pid, WEXITSTATUS(status));
		} else {
			log_debugx("child process %d terminated gracefully",
			    pid);
		}
		num++;
	}

	return num;
}

__dead static void
usage_automountd(void)
{

	fprintf(stderr, "Usage: %s [-D name=value][-m maxproc]"
	    "[-o opts][-Tidv]\n", getprogname());
	exit(EXIT_FAILURE);
}

static int
load_autofs(void)
{
	modctl_load_t args = {
		.ml_filename = "autofs",
		.ml_flags = MODCTL_NO_PROP,
		.ml_props = NULL,
		.ml_propslen = 0
	};
	int error;

	error = modctl(MODCTL_LOAD, &args);
	if (error && errno != EEXIST)
		log_warn("failed to load %s: %s", args.ml_filename,
		    strerror(errno));

	return error;
}

int
main_automountd(int argc, char **argv)
{
	pid_t pid;
	char *options = NULL;
	struct autofs_daemon_request request;
	int ch, debug = 0, error, maxproc = 30, saved_errno;
	bool dont_daemonize = false, incomplete_hierarchy = false;

	defined_init();

	while ((ch = getopt(argc, argv, "D:Tdim:o:v")) != -1) {
		switch (ch) {
		case 'D':
			defined_parse_and_add(optarg);
			break;
		case 'T':
			/*
			 * For compatibility with other implementations,
			 * such as OS X.
			 */
			debug++;
			break;
		case 'd':
			dont_daemonize = true;
			debug++;
			break;
		case 'i':
			incomplete_hierarchy = true;
			break;
		case 'm':
			maxproc = atoi(optarg);
			break;
		case 'o':
			options = concat(options, ',', optarg);
			break;
		case 'v':
			debug++;
			break;
		case '?':
		default:
			usage_automountd();
		}
	}
	argc -= optind;
	if (argc != 0)
		usage_automountd();

	log_init(debug);

	/*
	 * XXX: Workaround for NetBSD.
	 * load_autofs() should be needed only if open(2) failed with ENXIO.
	 * We attempt to load autofs before open(2) to suppress below warning
	 * "module error: incompatible module class for `autofs' (3 != 2)",
	 * which comes from sys/miscfs/specfs/spec_vnops.c:spec_open().
	 * spec_open() tries to load autofs as MODULE_CLASS_DRIVER while autofs
	 * is of MODULE_CLASS_VFS.
	 */
	load_autofs();

	/*
	 * NetBSD needs to check ENXIO here, but might not need ENOENT.
	 */
	autofs_fd = open(AUTOFS_PATH, O_RDWR | O_CLOEXEC);
	if (autofs_fd < 0 && (errno == ENOENT || errno == ENXIO)) {
		saved_errno = errno;
		if (!load_autofs())
			autofs_fd = open(AUTOFS_PATH, O_RDWR | O_CLOEXEC);
		else
			errno = saved_errno;
	}
	if (autofs_fd < 0)
		log_err(1, "failed to open %s", AUTOFS_PATH);

	if (dont_daemonize == false) {
		if (daemon(0, 0) == -1) {
			log_warn("cannot daemonize");
			pidfile_clean();
			exit(EXIT_FAILURE);
		}
	} else {
		lesser_daemon();
	}

	/*
	 * Call pidfile(3) after daemon(3).
	 */
	if (pidfile(NULL) == -1) {
		if (errno == EEXIST)
			log_errx(1, "daemon already running");
		else if (errno == ENAMETOOLONG)
			log_errx(1, "pidfile name too long");
		log_err(1, "cannot create pidfile");
	}
	if (pidfile_lock(NULL) == -1)
		log_err(1, "cannot lock pidfile");

	register_sigchld();

	for (;;) {
		log_debugx("waiting for request from the kernel");

		memset(&request, 0, sizeof(request));
		error = ioctl(autofs_fd, AUTOFSREQUEST, &request);
		if (error != 0) {
			if (errno == EINTR) {
				nchildren -= wait_for_children(false);
				assert(nchildren >= 0);
				continue;
			}

			log_err(1, "AUTOFSREQUEST");
		}

		if (dont_daemonize) {
			log_debugx("not forking due to -d flag; "
			    "will exit after servicing a single request");
		} else {
			nchildren -= wait_for_children(false);
			assert(nchildren >= 0);

			while (maxproc > 0 && nchildren >= maxproc) {
				log_debugx("maxproc limit of %d child processes"
				    " hit; waiting for child process to exit",
				    maxproc);
				nchildren -= wait_for_children(true);
				assert(nchildren >= 0);
			}
			log_debugx("got request; forking child process #%d",
			    nchildren);
			nchildren++;

			pid = fork();
			if (pid < 0)
				log_err(1, "fork");
			if (pid > 0)
				continue;
		}

		handle_request(&request, options, incomplete_hierarchy);
	}

	pidfile_clean();

	return EXIT_SUCCESS;
}
