#	$NetBSD: Makefile,v 1.2 2010/05/07 21:54:07 christos Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# setuid
PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c pw_dup.c
CPPFLAGS+=-I${.CURDIR} -DDEBUGGING=1
BINOWN =root
BINMODE=4555
BINDIR=/usr/bin
MAN=	crontab.1 crontab.5

.include <bsd.prog.mk>
