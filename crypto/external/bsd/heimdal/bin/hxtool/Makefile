# $NetBSD: Makefile,v 1.6.2.1 2025/08/02 05:18:39 perseant Exp $

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
