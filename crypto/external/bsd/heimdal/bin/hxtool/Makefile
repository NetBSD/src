# $NetBSD: Makefile,v 1.2.56.1 2023/08/11 13:39:44 martin Exp $

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

LDADD+= -lcrypto -ledit -lterminfo
DPADD+= ${LIBCRYPTO} ${LIBEDIT} ${LIBTERMINFO}
