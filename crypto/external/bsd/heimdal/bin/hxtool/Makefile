# $NetBSD: Makefile,v 1.4 2023/06/20 00:30:16 mrg Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/lib/hx509

CPPFLAGS+=-I${HEIMDIST}/lib

PROG= hxtool

HEIMSRCS= hxtool.c hxtool-commands.in

MAN=

LDADD+= -lgssapi
LDADD+= -lsl
LDADD+= -ledit -lterminfo
DPADD+= ${LIBGSSAPI}
DPADD+= ${LIBSL}
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

COPTS.hxtool.c+= -Wno-error=deprecated-declarations

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
