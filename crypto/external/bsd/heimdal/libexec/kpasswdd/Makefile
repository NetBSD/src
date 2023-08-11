# $NetBSD: Makefile,v 1.2.56.1 2023/08/11 13:40:08 martin Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/kpasswd

PROG=	kpasswdd
SRCS=	kpasswdd.c
MAN=	kpasswdd.8

CPPFLAGS+= -DINETD_SUPPORT
CPPFLAGS+= -I${HEIMDIST}/lib

PROGDPLIBS+= ${DPLIBASN1} ${DPLIBKADM5SRV} ${DPLIBHDB}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= -lutil ${KRB5LDADD}
DPADD+= ${LIBUTIL} ${KRB5DPADD}
