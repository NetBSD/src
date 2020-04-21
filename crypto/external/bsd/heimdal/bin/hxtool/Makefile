# $NetBSD: Makefile,v 1.2.46.2 2020/04/21 19:37:55 martin Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/lib/hx509

PROG= hxtool

HEIMSRCS= hxtool.c hxtool-commands.in

MAN=

LDADD+= -lgssapi
LDADD+= -lsl
LDADD+= -ledit -lterminfo
DPADD+= ${LIBGSSAPI}
DPADD+= ${LIBSL}
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
