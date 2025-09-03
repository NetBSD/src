# $NetBSD: Makefile,v 1.7 2025/07/20 16:47:16 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/lib/hx509

PROG= hxtool

HEIMSRCS= hxtool.c hxtool-commands.in

MAN=

PROGDPLIBS+= ${DPLIBGSSAPI} ${DPLIBSL}

COPTS.hxtool.c+= -Wno-error=deprecated-declarations

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= -lcrypto -ledit -lterminfo -lpthread
DPADD+= ${LIBCRYPTO} ${LIBEDIT} ${LIBTERMINFO} ${LIBPTHREAD}
