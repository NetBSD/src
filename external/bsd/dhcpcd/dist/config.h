/* $NetBSD: config.h,v 1.1.1.26.2.1 2014/12/29 16:18:04 martin Exp $ */

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
