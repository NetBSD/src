# $NetBSD: Makefile,v 1.3 2023/06/19 23:11:44 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/kpasswd ${HEIMDIST}/lib/hcrypto

PROG=	kpasswdd
SRCS=	kpasswdd.c ui.c
MAN=	kpasswdd.8

CPPFLAGS+= -DINETD_SUPPORT
CPPFLAGS+= -I${HEIMDIST}/lib

LDADD+= -lkadm5srv -lhdb -lutil
DPADD+= ${LIBKADM5SRV} ${LIBHDB} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
