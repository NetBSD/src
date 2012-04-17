# $NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:01:58 yamt Exp $
.include "../Makefile.inc"

.PATH.c: ${DIST}/src/tcsd

CPPFLAGS+=-DAPPID=\"TCSD\" -DVAR_PREFIX=\"/var\"  -DETC_PREFIX=\"/etc\"
CPPFLAGS+=-DTSS_BUILD_PS -DTSS_BUILD_PCR_EVENTS

BINDIR=/usr/sbin
PROG=tcsd
SRCS=\
platform.c \
svrside.c \
tcsd_conf.c \
tcsd_threads.c 

LIBTCSOBJDIR!=	cd ${.CURDIR}/../../lib/libtcs && ${PRINTOBJDIR}

DPADD+=${LIBTDDL} ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=-L${LIBTCSOBJDIR} -ltcs -ltddl -lcrypto -lpthread

.SUFFIXES: .5 .8
.PATH.5: ${DIST}/man/man5
.PATH.8: ${DIST}/man/man8

MAN+=tcsd.conf.5 tcsd.8

.include <bsd.prog.mk>
