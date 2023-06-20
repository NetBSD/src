# $NetBSD: Makefile,v 1.4 2023/06/20 17:23:03 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/lib/kadm5

PROG=		ipropd-master

MAN=		iprop.8
MLINKS=	\
	iprop.8		ipropd-master.8		\
	iprop.8		ipropd-slave.8

SRCS+= 	ipropd_master.c				\
	ipropd_common.c

CPPFLAGS+= 				\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/asn1		\
	-I${DESTDIR}/usr/include/kadm5

PROGDPLIBS+= ${DPLIBKADM5SRV} ${DPLIBHDB}
LDADD+= -lutil
DPADD+= ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
