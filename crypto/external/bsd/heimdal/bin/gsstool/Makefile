# $NetBSD: Makefile,v 1.2.46.2 2020/04/21 19:37:55 martin Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/lib/gssapi

PROG= gsstool

HEIMSRCS=	gsstool.c gss-commands.in

MAN=

LDADD+= -lgssapi -lheimntlm
LDADD+= -lsl ${LIBVERS} 
LDADD+= -ledit -lterminfo
DPADD+= ${LIBGSSAPI} ${LIBHEIMNTLM}
DPADD+= ${LIBSL}
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
