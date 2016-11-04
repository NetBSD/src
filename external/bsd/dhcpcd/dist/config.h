/* $NetBSD: config.h,v 1.11.2.1 2016/11/04 14:42:45 pgoyette Exp $ */

/* netbsd */
#define SYSCONFDIR	"/etc"
#define SBINDIR		"/sbin"
#define LIBDIR		"/lib"
#define LIBEXECDIR	"/libexec"
#define DBDIR		"/var/db"
#define RUNDIR		"/var/run"
#define HAVE_IFAM_PID
#define HAVE_IFAM_ADDRFLAGS
#define HAVE_IFADDRS_ADDRFLAGS
#define HAVE_UTIL_H
#define HAVE_SYS_QUEUE_H
#define HAVE_SPAWN_H
#define HAVE_REALLOCARRAY
#define HAVE_KQUEUE
#define HAVE_KQUEUE1
#define HAVE_SYS_BITOPS_H
#define HAVE_MD5_H
#define SHA2_H		<sha2.h>
