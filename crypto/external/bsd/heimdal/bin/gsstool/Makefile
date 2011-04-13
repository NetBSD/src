# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:44 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/lib/gssapi

PROG= gsstool

HEIMSRCS=	gsstool.c gss-commands.in

MAN=

LDADD+=	-lgssapi -lsl ${LIBVERS}
DPADD+=	${LIBGSSAPI} ${LIBSL} ${LIBVERS}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
