# $NetBSD: Makefile,v 1.3 2023/08/10 06:56:12 riastradh Exp $
.include "../Makefile.inc"

.PATH.c: ${DIST}/src/tcsd

CPPFLAGS+=-DAPPID=\"TCSD\" -DVAR_PREFIX=\"/var\"  -DETC_PREFIX=\"/etc\"
CPPFLAGS+=-DTSS_BUILD_PS -DTSS_BUILD_PCR_EVENTS

COPTS.tcsd_threads.c+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

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

.SUFFIXES: .in
.PATH.in: ${DIST}/man/man5 ${DIST}/man/man8

MAN+=tcsd.conf.5 tcsd.8
CLEANFILES+= tcsd.conf.5 tcsd.8

__mansed: .USE
	@${TOOL_SED} -e s/@PACKAGE_BUGREPORT@/gnats-bugs@NetBSD.org/g \
		-e s/@TCSD_DEFAULT_PORT@/30003/g ${.ALLSRC} > ${.TARGET}

tcsd.8: tcsd.8.in __mansed
tcsd.conf.5: tcsd.conf.5.in __mansed

.include <bsd.prog.mk>
