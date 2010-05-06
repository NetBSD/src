#	$NetBSD: Makefile,v 1.1 2010/05/06 18:58:45 christos Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# setuid
PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c pw_dup.c
CPPFLAGS+=-I${.CURDIR} -DDEBUGGING=1
BINOWN =root
BINMODE=4555
MAN=	crontab.1 crontab.5

.include <bsd.prog.mk>
