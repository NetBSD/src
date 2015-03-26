/* $NetBSD: config.h,v 1.8 2015/03/26 10:26:37 roy Exp $ */

/* netbsd */
#define SYSCONFDIR	"/etc"
#define SBINDIR		"/sbin"
#define LIBDIR		"/lib"
#define LIBEXECDIR	"/libexec"
#define DBDIR		"/var/db"
#define RUNDIR		"/var/run"
#include		<sys/queue.h>
#define HAVE_SPAWN_H
#define HAVE_KQUEUE
#define HAVE_KQUEUE1
#define HAVE_MD5_H
#define SHA2_H		<sha2.h>
