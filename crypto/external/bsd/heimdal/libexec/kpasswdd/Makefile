# $NetBSD: Makefile,v 1.4 2023/06/20 17:23:03 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/kpasswd

PROG=	kpasswdd
SRCS=	kpasswdd.c
MAN=	kpasswdd.8

CPPFLAGS+= -DINETD_SUPPORT
CPPFLAGS+= -I${HEIMDIST}/lib

PROGDPLIBS+= ${DPLIBASN1} ${DPLIBKADM5SRV} ${DPLIBHDB}
LDADD+= -lcrypto -lutil
DPADD+= ${LIBCRYPTO} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
