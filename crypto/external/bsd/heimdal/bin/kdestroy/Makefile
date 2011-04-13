# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:45 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kuser

PROG= kdestroy
SRCS= kdestroy.c

LDADD+=	-lkrb5 -lkafs -lsl -lheimbase -lheimntlm ${LIBVERS}
DPADD+=	${LIBKRB5} ${LIBKAFS} ${LIBSL} ${LIBVERS}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
