# $NetBSD: Makefile,v 1.5 2023/06/20 17:23:01 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/lib/hx509

PROG= hxtool

HEIMSRCS= hxtool.c hxtool-commands.in

MAN=

PROGDPLIBS+= ${DPLIBASN1} ${DPLIBGSSAPI} ${DPLIBHX509} ${DPLIBSL}
LDADD+= -lcrypto -ledit -lterminfo
DPADD+= ${LIBCRYPTO} ${LIBEDIT} ${LIBTERMINFO}

COPTS.hxtool.c+= -Wno-error=deprecated-declarations

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
