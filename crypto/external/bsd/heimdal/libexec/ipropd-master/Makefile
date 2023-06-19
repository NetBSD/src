# $NetBSD: Makefile,v 1.3 2023/06/19 23:11:44 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/lib/kadm5 ${HEIMDIST}/lib/hcrypto

PROG=		ipropd-master

MAN=		iprop.8
MLINKS=	\
	iprop.8		ipropd-master.8		\
	iprop.8		ipropd-slave.8

SRCS+= 	ipropd_master.c				\
	ipropd_common.c				\
	ui.c

CPPFLAGS+= 				\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/asn1		\
	-I${HEIMDIST}/lib		\
	-I${DESTDIR}/usr/include/kadm5

LDADD+= -lkadm5srv -lhdb -lutil
DPADD+= ${LIBKADM5SRV} ${LIBHDB} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
