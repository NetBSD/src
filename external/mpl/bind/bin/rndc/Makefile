#	$NetBSD: Makefile,v 1.2.6.1 2024/02/25 15:42:54 martin Exp $

.include <bsd.init.mk>

PROG=	rndc
SRCS=	rndc.c util.c
MAN=	rndc.8 rndc.conf.5
BINDIR=	/usr/sbin

DIST=${IDIST}/bin/rndc
CPPFLAGS+= -I${DIST}/include \
        -DRNDC_CONFFILE=\"${SYSCONFDIR}/rndc.conf\" \
	-DRNDC_KEYFILE=\"${SYSCONFDIR}/rndc.key\" \

.PATH: ${DIST} ${IDIST}/doc/man

.include <bsd.prog.mk>
