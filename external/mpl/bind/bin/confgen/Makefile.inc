#	$NetBSD: Makefile.inc,v 1.2 2020/08/03 17:23:35 christos Exp $

PROG=	${.CURDIR:T}
SRCS=	${PROG}.c keygen.c util.c os.c
MAN=	${PROG}.8
BINDIR=	/usr/sbin

.include "${.PARSEDIR}/../Makefile.inc"

DIST=${IDIST}/bin/confgen

CPPFLAGS+= -I${DIST}/include \
        -DRNDC_CONFFILE=\"${SYSCONFDIR}/rndc.conf\" \
	-DRNDC_KEYFILE=\"${SYSCONFDIR}/rndc.key\" \


.PATH: ${DIST} ${DIST}/unix ${IDIST}/doc/man
