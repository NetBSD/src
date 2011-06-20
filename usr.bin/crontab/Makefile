#	$NetBSD: Makefile,v 1.27 2011/06/20 07:44:01 mrg Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# setuid
PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c
CPPFLAGS+=-I${.CURDIR} -I${NETBSDSRCDIR}/usr.sbin/cron -DDEBUGGING=1
CPPFLAGS+=-DUSE_UTIMES
BINOWN =root
BINMODE=4555
MAN=	crontab.1 crontab.5

.PATH: ${NETBSDSRCDIR}/usr.sbin/cron

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.misc.c+=	-Wno-pointer-sign
.endif
