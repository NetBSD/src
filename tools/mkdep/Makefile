#	$NetBSD: Makefile,v 1.5 2001/12/12 02:42:17 tv Exp $

HOSTPROGNAME=	nbmkdep
HOST_SRCDIR=	usr.bin/mkdep
NOCOMPATLIB=	# defined

.PATH:		${.CURDIR}/../compat
SRCS=		mkdep.c setprogname.c

.include "${.CURDIR}/../Makefile.host"

# Don't run mkdep on mkdep.
MKDEP=		touch $@ || true

# This is the only program that comes before binstall.
INSTALL=	true
${HOST_BINDIR}/${HOSTPROGNAME}:: mkdep
	-rm -f $@
	cp mkdep $@
