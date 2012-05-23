#	$NetBSD: Makefile.inc,v 1.1.4.2 2012/05/23 10:07:22 yamt Exp $

PROG=	${.CURDIR:T}
SRCS=	${PROG}.c keygen.c util.c os.c
MAN=	${PROG}.8
BINDIR=	/usr/sbin

.include "${.PARSEDIR}/../Makefile.inc"

DIST=${IDIST}/bin/confgen

CPPFLAGS+= -I${DIST}/include \
        -DRNDC_CONFFILE=\"${SYSCONFDIR}/rndc.conf\" \
	-DRNDC_KEYFILE=\"${SYSCONFDIR}/rndc.key\" \


.PATH: ${DIST} ${DIST}/unix
