/*++
/* NAME
/*	chroot_uid 3
/* SUMMARY
/*	limit possible damage a process can do
/* SYNOPSIS
/*	#include <chroot_uid.h>
/*
/*	void	chroot_uid(root_dir, user_name)
/*	const char *root_dir;
/*	const char *user_name;
/* DESCRIPTION
/*	\fBchroot_uid\fR changes the process root to \fIroot_dir\fR and
/*	changes process privileges to those of \fIuser_name\fR.
/* DIAGNOSTICS
/*	System call errors are reported via the msg(3) interface.
/*	All errors are fatal.
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
#include <pwd.h>
#include <unistd.h>
#include <grp.h>

/* Utility library. */

#include "msg.h"
#include "chroot_uid.h"

/* chroot_uid - restrict the damage that this program can do */

void    chroot_uid(const char *root_dir, const char *user_name)
{
    struct passwd *pwd;
    uid_t   uid;
    gid_t   gid;

    /*
     * Look up the uid/gid before entering the jail, and save them so they
     * can't be clobbered. Set up the primary and secondary groups.
     */
    if (user_name != 0) {
	if ((pwd = getpwnam(user_name)) == 0)
	    msg_fatal("unknown user: %s", user_name);
	uid = pwd->pw_uid;
	gid = pwd->pw_gid;
	if (setgid(gid) < 0)
	    msg_fatal("setgid(%ld): %m", (long) gid);
	if (initgroups(user_name, gid) < 0)
	    msg_fatal("initgroups: %m");
    }

    /*
     * Enter the jail.
     */
    if (root_dir) {
	if (chroot(root_dir))
	    msg_fatal("chroot(%s): %m", root_dir);
	if (chdir("/"))
	    msg_fatal("chdir(/): %m");
    }

    /*
     * Drop the user privileges.
     */
    if (user_name != 0)
	if (setuid(uid) < 0)
	    msg_fatal("setuid(%ld): %m", (long) uid);

    /*
     * Give the desperate developer a clue of what is happening.
     */
    if (msg_verbose > 1)
	msg_info("chroot %s user %s",
		 root_dir ? root_dir : "(none)",
		 user_name ? user_name : "(none)");
}
