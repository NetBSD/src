# $NetBSD: Makefile,v 1.3 2023/06/20 17:23:01 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/lib/gssapi

PROG= gsstool

HEIMSRCS=	gsstool.c gss-commands.in

MAN=

PROGDPLIBS += ${DPLIBGSSAPI} ${DPLIBSL}

LDADD+= -ledit -lterminfo
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
