/*	$NetBSD: fsspace.c,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

/*++
/* NAME
/*	fsspace 3
/* SUMMARY
/*	determine available file system space
/* SYNOPSIS
/*	#include <fsspace.h>
/*
/*	struct fsspace {
/* .in +4
/*		unsigned long block_size;
/*		unsigned long block_free;
/* .in -4
/*	};
/*
/*	void	fsspace(path, sp)
/*	const char *path;
/*	struct fsspace *sp;
/* DESCRIPTION
/*	fsspace() returns the amount of available space in the file
/*	system specified in \fIpath\fR, in terms of the block size and
/*	of the number of available blocks.
/* DIAGNOSTICS
/*	All errors are fatal.
/* BUGS
/*	Use caution when doing computations with the result from fsspace().
/*	It is easy to cause overflow (by multiplying large numbers) or to
/*	cause underflow (by subtracting unsigned numbers).
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

#if defined(STATFS_IN_SYS_MOUNT_H)
#include <sys/param.h>
#include <sys/mount.h>
#elif defined(STATFS_IN_SYS_VFS_H)
#include <sys/vfs.h>
#elif defined(STATVFS_IN_SYS_STATVFS_H)
#include <sys/statvfs.h>
#elif defined(STATFS_IN_SYS_STATFS_H)
#include <sys/statfs.h>
#else
#ifdef USE_STATFS
#error "please specify the include file with `struct statfs'"
#else
#error "please specify the include file with `struct statvfs'"
#endif
#endif

/* Utility library. */

#include <msg.h>
#include <fsspace.h>

/* fsspace - find amount of available file system space */

void    fsspace(const char *path, struct fsspace * sp)
{
    const char *myname = "fsspace";

#ifdef USE_STATFS
#ifdef USE_STRUCT_FS_DATA			/* Ultrix */
    struct fs_data fsbuf;

    if (statfs(path, &fsbuf) < 0)
	msg_fatal("statfs %s: %m", path);
    sp->block_size = 1024;
    sp->block_free = fsbuf.fd_bfreen;
#else
    struct statfs fsbuf;

    if (statfs(path, &fsbuf) < 0)
	msg_fatal("statfs %s: %m", path);
    sp->block_size = fsbuf.f_bsize;
    sp->block_free = fsbuf.f_bavail;
#endif
#endif
#ifdef USE_STATVFS
    struct statvfs fsbuf;

    if (statvfs(path, &fsbuf) < 0)
	msg_fatal("statvfs %s: %m", path);
    sp->block_size = fsbuf.f_frsize;
    sp->block_free = fsbuf.f_bavail;
#endif
    if (msg_verbose)
	msg_info("%s: %s: block size %lu, blocks free %lu",
		 myname, path, sp->block_size, sp->block_free);
}

#ifdef TEST

 /*
  * Proof-of-concept test program: print free space unit and count for all
  * listed file systems.
  */

#include <vstream.h>

int     main(int argc, char **argv)
{
    struct fsspace sp;

    if (argc == 1)
	msg_fatal("usage: %s filesystem...", argv[0]);

    while (--argc && *++argv) {
	fsspace(*argv, &sp);
	vstream_printf("%10s: block size %lu, blocks free %lu\n",
		       *argv, sp.block_size, sp.block_free);
	vstream_fflush(VSTREAM_OUT);
    }
    return (0);
}

#endif
