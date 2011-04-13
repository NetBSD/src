# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:58 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/lib/kadm5

PROG= iprop-log
MAN=  iprop-log.8

HEIMSRCS= iprop-log.c iprop-commands.in

CPPFLAGS+= -I${DESTDIR}/usr/include/kadm5

LDADD+= -lkadm5srv -lhdb -lkrb5 -lhx509 -lcrypto \
	-lasn1 -lsl -lcom_err ${LIBVERS} \
	-lroken -lcrypt -ledit -lterminfo
DPADD+= ${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBCRYPTO}	\
	${LIBASN1} ${LIBSL} ${LIBCOM_ERR} ${LIBVERS}			\
	${LIBROKEN} ${LIBCRYPT} ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
