# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:57 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/lib/kadm5

PROG=		ipropd-slave

# man page included with ipropd-master
MAN=

SRCS+= 	ipropd_slave.c ipropd_common.c

CPPFLAGS+= 				\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/asn1		\
	-I${DESTDIR}/usr/include/kadm5

LDADD+=		-lkadm5srv -lhdb -lkrb5 -lhx509 -lasn1 \
		-lcrypto ${LIBVERS} -lcom_err -lroken \
		-lcrypt -lutil
DPADD+=		${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBASN1} \
		${LIBCRYPTO} ${LIBVERS} ${LIBCOM_ERR} ${LIBROKEN} \
		${LIBCRYPT} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
