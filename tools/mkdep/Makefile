#	$NetBSD: Makefile,v 1.4 2001/12/11 21:18:01 tv Exp $

HOSTPROGNAME=	nbmkdep
HOST_SRCDIR=	usr.bin/mkdep
NOCOMPATLIB=	# defined

.PATH:		${.CURDIR}/../compat
SRCS=		mkdep.c setprogname.c

.include "${.CURDIR}/../Makefile.host"

# Don't run mkdep on mkdep.
MKDEP=		touch $@ || true

# This is the only program that comes before binstall.
INSTALL=	@true
${HOST_BINDIR}/${HOSTPROGNAME}:: mkdep
	-rm -f $@
	cp mkdep $@
