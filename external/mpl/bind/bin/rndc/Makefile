#	$NetBSD: Makefile,v 1.2 2020/08/03 17:23:36 christos Exp $

.include <bsd.init.mk>

PROG=	rndc
SRCS=	rndc.c util.c
MAN=	rndc.8 rndc.conf.5
BINDIR=	/usr/sbin

DIST=${IDIST}/bin/rndc
CPPFLAGS+= -I${DIST}/include \
        -DRNDC_CONFFILE=\"${SYSCONFDIR}/rndc.conf\" \
	-DRNDC_KEYFILE=\"${SYSCONFDIR}/rndc.key\" \

rndc.o: ${VERSIONFILE}

.PATH: ${DIST} ${IDIST}/doc/man

.include <bsd.prog.mk>
