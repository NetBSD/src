#	$NetBSD: Makefile,v 1.3 2017/06/09 17:36:29 christos Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# setuid
PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c pw_dup.c closeall.c
CPPFLAGS+=-I${.CURDIR} -DDEBUGGING=1
BINOWN =root
BINMODE=4555
BINDIR=/usr/bin
MAN=	crontab.1 crontab.5

.include <bsd.prog.mk>
