/*++
/* NAME
/*	postsuper 1
/* SUMMARY
/*	Postfix super intendent
/* SYNOPSIS
/* .fi
/*	\fBpostsuper\fR [\fB-p\fR] [\fB-s\fR] [\fB-v\fR] [\fIdirectory ...\fR]
/* DESCRIPTION
/*	The \fBpostsuper\fR command does small maintenance jobs on the named
/*	Postfix queue directories (default: all).
/*	Directory names are relative to the Postfix top-level queue directory.
/*
/*	By default, \fBpostsuper\fR performs the operations requested with the
/*	\fB-s\fR and \fB-p\fR command-line options.
/*	\fBpostsuper\fR always tries to remove objects that are neither files
/*	nor directories.  Use of this command is restricted to the super-user.
/*
/*	Options:
/* .IP \fB-s\fR
/*	Structure check.  Move queue files that are in the wrong place
/*	in the file system hierarchy and remove subdirectories that are
/*	no longer needed. File rearrangements are necessary after a change
/*	in the \fBhash_queue_names\fR and/or \fBhash_queue_depth\fR
/*	configuration parameters. It is highly recommended to run this
/*	check once before Postfix startup.
/* .IP \fB-p\fR
/*	Purge stale files (files that are left over after system or
/*	software crashes).
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream and to
/*	\fBsyslogd\fR.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	See the Postfix \fBmain.cf\fR file for syntax details and for
/*	default values.
/* .IP \fBhash_queue_depth\fR
/*	Number of subdirectory levels for hashed queues.
/* .IP \fBhash_queue_names\fR
/*	The names of queues that are organized into multiple levels of
/*	subdirectories.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>			/* remove() */

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <msg_syslog.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <scan_dir.h>
#include <vstring.h>
#include <safe.h>
#include <set_ugid.h>
#include <argv.h>

/* Global library. */

#include <mail_task.h>
#include <mail_conf.h>
#include <mail_params.h>
#include <mail_queue.h>

/* Application-specific. */

#define MAX_TEMP_AGE (60 * 60 * 24)	/* temp file maximal age */
#define STR vstring_str			/* silly little macro */

#define ACTION_STRUCT	(1<<0)		/* fix file organization */
#define ACTION_PURGE	(1<<1)		/* purge old temp files */

#define ACTION_DEFAULT	(ACTION_STRUCT | ACTION_PURGE)

 /*
  * Information about queue directories and what we expect to do there. If a
  * file has unexpected owner permissions and is older than some threshold,
  * the file is discarded. We don't step into maildrop subdirectories - if
  * maildrop is writable, we might end up in the wrong place, deleting the
  * wrong information.
  */
struct queue_info {
    char   *name;			/* directory name */
    int     perms;			/* expected permissions */
    int     flags;			/* see below */
};

#define RECURSE		(1<<0)		/* step into subdirectories */
#define	DONT_RECURSE	0		/* don't step into directories */

static struct queue_info queue_info[] = {
    MAIL_QUEUE_MAILDROP, MAIL_QUEUE_STAT_READY, DONT_RECURSE,
    MAIL_QUEUE_INCOMING, MAIL_QUEUE_STAT_READY, RECURSE,
    MAIL_QUEUE_ACTIVE, MAIL_QUEUE_STAT_READY, RECURSE,
    MAIL_QUEUE_DEFERRED, MAIL_QUEUE_STAT_READY, RECURSE,
    MAIL_QUEUE_DEFER, 0600, RECURSE,
    MAIL_QUEUE_BOUNCE, 0600, RECURSE,
    MAIL_QUEUE_FLUSH, 0600, RECURSE,
    0,
};

/* super - check queue file location and clean up */

static void super(char **queues, int action)
{
    ARGV   *hash_queue_names = argv_split(var_hash_queue_names, " \t\r\n,");
    VSTRING *actual_path = vstring_alloc(10);
    VSTRING *wanted_path = vstring_alloc(10);
    struct stat st;
    char   *queue_name;
    SCAN_DIR *info;
    char   *path;
    int     actual_depth;
    int     wanted_depth;
    char  **cpp;
    struct queue_info *qp;

    /*
     * Make sure every file is in the right place, clean out stale files, and
     * remove non-file/non-directory objects.
     */
    while ((queue_name = *queues++) != 0) {

	if (msg_verbose)
	    msg_info("queue: %s", queue_name);

	/*
	 * Look up queue-specific properties: desired hashing depth, what
	 * file permissions to look for, and whether or not it is desirable
	 * to step into subdirectories.
	 */
	for (qp = queue_info; /* void */ ; qp++) {
	    if (qp->name == 0)
		msg_fatal("unknown queue name: %s", queue_name);
	    if (strcmp(qp->name, queue_name) == 0)
		break;
	}
	for (cpp = hash_queue_names->argv; /* void */ ; cpp++) {
	    if (*cpp == 0) {
		wanted_depth = 0;
		break;
	    }
	    if (strcmp(*cpp, queue_name) == 0) {
		wanted_depth = var_hash_queue_depth;
		break;
	    }
	}

	/*
	 * Sanity check. Some queues just cannot be recursive.
	 */
	if (wanted_depth > 0 && (qp->flags & RECURSE) == 0)
	    msg_fatal("%s queue must not be hashed", queue_name);

	/*
	 * Other per-directory initialization.
	 */
	info = scan_dir_open(queue_name);
	actual_depth = 0;

	for (;;) {

	    /*
	     * If we reach the end of a subdirectory, return to its parent.
	     * Delete subdirectories that are no longer needed.
	     */
	    if ((path = scan_dir_next(info)) == 0) {
		if (actual_depth == 0)
		    break;
		if (actual_depth > wanted_depth) {
		    if (rmdir(scan_dir_path(info)) < 0 && errno != ENOENT)
			msg_warn("remove %s: %m", scan_dir_path(info));
		    else if (msg_verbose)
			msg_info("remove %s", scan_dir_path(info));
		}
		scan_dir_pop(info);
		actual_depth--;
		continue;
	    }

	    /*
	     * If we stumble upon a subdirectory, enter it, if it is
	     * considered safe to do so. Otherwise, try to remove the
	     * subdirectory at a later stage.
	     */
	    if (strlen(path) == 1 && (qp->flags & RECURSE) != 0) {
		actual_depth++;
		scan_dir_push(info, path);
		continue;
	    }

	    /*
	     * See if this is a stale file or some non-file object. Be
	     * careful not to delete bounce or defer logs just because they
	     * are more than a couple days old.
	     */
	    vstring_sprintf(actual_path, "%s/%s", scan_dir_path(info), path);
	    if (stat(STR(actual_path), &st) < 0)
		continue;
	    if (S_ISDIR(st.st_mode)) {
		if (rmdir(STR(actual_path)) < 0 && errno != ENOENT)
		    msg_warn("remove subdirectory %s: %m", STR(actual_path));
		else if (msg_verbose)
		    msg_info("remove subdirectory %s", STR(actual_path));
		continue;
	    }
	    if (!S_ISREG(st.st_mode)
		|| ((action & ACTION_PURGE) != 0 &&
		    (st.st_mode & S_IRWXU) != qp->perms
		    && time((time_t *) 0) > st.st_mtime + MAX_TEMP_AGE)) {
		if (remove(STR(actual_path)) < 0 && errno != ENOENT)
		    msg_warn("remove %s: %m", STR(actual_path));
		else if (msg_verbose)
		    msg_info("remove %s", STR(actual_path));
		continue;
	    }

	    /*
	     * Skip over files with illegal names. The library routines
	     * refuse to operate on them.
	     */
	    if (mail_queue_id_ok(path) == 0)
		continue;

	    /*
	     * Skip temporary files that aren't old enough.
	     */
	    if (qp->perms == MAIL_QUEUE_STAT_READY
		&& (st.st_mode & S_IRWXU) != qp->perms)
		continue;

	    /*
	     * See if this file sits in the right place in the file system
	     * hierarchy. Its place may be wrong after a change to the
	     * hash_queue_{names,depth} parameter settings. The implied
	     * mkdir() operation is the main reason for this command to run
	     * with postfix privilege. The mail_queue_mkdirs() routine could
	     * be fixed to use the "right" privilege, but it is a good idea
	     * to do everything with the postfix owner privileges regardless,
	     * in order to limit the amount of damage that we can do.
	     */
	    (void) mail_queue_path(wanted_path, queue_name, path);
	    if (strcmp(STR(actual_path), STR(wanted_path)) != 0) {
		if (rename(STR(actual_path), STR(wanted_path)) < 0)
		    if (errno != ENOENT
			|| mail_queue_mkdirs(STR(wanted_path)) < 0
			|| rename(STR(actual_path), STR(wanted_path)) < 0)
			msg_fatal("rename %s to %s: %m", STR(actual_path),
				  STR(wanted_path));
		if (msg_verbose)
		    msg_info("rename %s to %s", STR(actual_path),
			     STR(wanted_path));
	    }
	}
	scan_dir_close(info);
    }

    /*
     * Clean up.
     */
    vstring_free(wanted_path);
    vstring_free(actual_path);
    argv_free(hash_queue_names);
}

int     main(int argc, char **argv)
{
    int     fd;
    struct stat st;
    char   *slash;
    int     debug_me = 0;
    int     action = 0;
    char  **queues;
    int     c;

    /*
     * Defaults.
     */
    static char *default_queues[] = {
	MAIL_QUEUE_MAILDROP,
	MAIL_QUEUE_INCOMING,
	MAIL_QUEUE_ACTIVE,
	MAIL_QUEUE_DEFERRED,
	MAIL_QUEUE_DEFER,
	MAIL_QUEUE_BOUNCE,
	MAIL_QUEUE_FLUSH,
	0,
    };

    /*
     * Be consistent with file permissions.
     */
    umask(022);

    /*
     * To minimize confusion, make sure that the standard file descriptors
     * are open before opening anything else. XXX Work around for 44BSD where
     * fstat can return EBADF on an open file descriptor.
     */
    for (fd = 0; fd < 3; fd++)
	if (fstat(fd, &st) == -1
	    && (close(fd), open("/dev/null", O_RDWR, 0)) != fd)
	    msg_fatal("open /dev/null: %m");

    /*
     * Process environment options as early as we can. We might be called
     * from a set-uid (set-gid) program, so be careful with importing
     * environment variables.
     */
    if (safe_getenv(CONF_ENV_VERB))
	msg_verbose = 1;
    if (safe_getenv(CONF_ENV_DEBUG))
	debug_me = 1;

    /*
     * Initialize. Set up logging, read the global configuration file and
     * extract configuration information.
     */
    if ((slash = strrchr(argv[0], '/')) != 0)
	argv[0] = slash + 1;
    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_syslog_init(mail_task(argv[0]), LOG_PID, LOG_FACILITY);
    set_mail_conf_str(VAR_PROCNAME, var_procname = mystrdup(argv[0]));

    mail_conf_read();
    if (chdir(var_queue_dir))
	msg_fatal("chdir %s: %m", var_queue_dir);

    /*
     * All file/directory updates must be done as the mail system owner. This
     * is because Postfix daemons manipulate the queue with those same
     * privileges, so directories must be created with the right ownership.
     * 
     * Running as a non-root user is also required for security reasons. When
     * the Postfix queue hierarchy is compromised, an attacker could trick us
     * into entering other file hierarchies and afflicting damage. Running as
     * a non-root user limits the damage to the already compromised mail
     * owner.
     */
    set_ugid(var_owner_uid, var_owner_gid);

    /*
     * Parse JCL.
     */
    while ((c = GETOPT(argc, argv, "spv")) > 0) {
	switch (c) {
	default:
	    msg_fatal("usage: %s [-s (fix structure)] [-p (purge stale files)]",
		      argv[0]);
	case 's':
	    action |= ACTION_STRUCT;
	    break;
	case 'p':
	    action |= ACTION_PURGE;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	}
    }

    /*
     * Execute the explicitly specified (or default) action, on the
     * explicitly specified (or default) queues.
     */
    if (action == 0)
	action = ACTION_DEFAULT;
    if (argv[optind] == 0)
	queues = default_queues;
    else
	queues = argv + optind;

    super(queues, action);

    exit(0);
}
