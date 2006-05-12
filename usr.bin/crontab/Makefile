#	$NetBSD: Makefile,v 1.24 2006/05/12 03:29:11 mrg Exp $

.include <bsd.own.mk>

PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c
CPPFLAGS+=-I${.CURDIR} -I${NETBSDSRCDIR}/usr.sbin/cron -DDEBUGGING=1
CPPFLAGS+=-DUSE_UTIMES
BINOWN =root
BINMODE=4555
MAN=	crontab.1 crontab.5

.PATH: ${NETBSDSRCDIR}/usr.sbin/cron

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.misc.c+=	-Wno-pointer-sign
.endif
