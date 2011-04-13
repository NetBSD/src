# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:45 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/lib/hx509

PROG= hxtool

HEIMSRCS= hxtool.c hxtool-commands.in

MAN=

LDADD+=	-lgssapi -lsl ${LIBVERS}
DPADD+=	${LIBGSSAPI} ${LIBSL} ${LIBVERS}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
