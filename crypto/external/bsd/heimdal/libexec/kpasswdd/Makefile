# $NetBSD: Makefile,v 1.2.56.2 2023/10/14 06:56:35 martin Exp $

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

LDADD+= -lutil ${KRB5LDADD} ${HDBLDADD}
DPADD+= ${LIBUTIL} ${KRB5DPADD} ${HDBDPADD}
