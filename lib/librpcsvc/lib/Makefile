#	$Id: Makefile,v 1.1 1993/10/08 05:26:38 cgd Exp $

RPCDIR=		${.CURDIR}/../src
.PATH:		${RPCDIR}

.include "${.CURDIR}/../Makefile.src"

LIB=		rpcsvc

CFLAGS+=	-I/usr/include/rpcsvc
SRCS=		${RPCSRCS:R:S/$/.c/g}
CLEANFILES+=	${SRCS}

NOMAN=		noman
NOPIC=

# new suffixes have to go afterwards, because bsd.lib.mk clears them
.include <bsd.lib.mk>
.include "${.CURDIR}/../Makefile.dep"
