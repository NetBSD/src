# $NetBSD: Makefile,v 1.3 2023/06/20 00:30:17 mrg Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/kuser

PROG=	kimpersonate
SRCS=	kimpersonate.c
MAN=	kimpersonate.8

CPPFLAGS+=				\
	-I${HEIMDIST}/lib

LDADD+= -lkafs -lheimntlm
DPADD+= ${LIBKAFS} ${LIBHEIMNTLM}

# XXX: Pull this in from hcrypto.  Be sure to be last!
SRCS+=	ui.c
.PATH:	${HEIMDIST}/lib/hcrypto
CPPFLAGS.ui.c+= 		\
	-I${HEIMDIST}/lib/hcrypto

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
