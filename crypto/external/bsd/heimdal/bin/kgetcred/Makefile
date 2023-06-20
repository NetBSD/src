# $NetBSD: Makefile,v 1.3 2023/06/20 00:30:16 mrg Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser

PROG= kgetcred
SRCS= kgetcred.c

LDADD+= -lkafs -lsl -lheimntlm
DPADD+= ${LIBKAFS} ${LIBSL} ${LIBHEIMNTLM}

CPPFLAGS+= -I${HEIMDIST}/lib

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
