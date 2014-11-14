/* $NetBSD: config.h,v 1.7 2014/11/14 12:00:54 roy Exp $ */

/* netbsd */
#define SYSCONFDIR	"/etc"
#define SBINDIR		"/sbin"
#define LIBDIR		"/lib"
#define LIBEXECDIR	"/libexec"
#define DBDIR		"/var/db"
#define RUNDIR		"/var/run"
#include		<sys/queue.h>
#define HAVE_SPAWN_H
#define HAVE_MD5_H
#define SHA2_H		<sha2.h>
