#	$Id: Makefile,v 1.3 1993/10/13 05:28:03 cgd Exp $

RPCDIR=		${.CURDIR}/../src
.PATH:		${RPCDIR}

.include "${.CURDIR}/../Makefile.src"

LIB=		rpcsvc

SRCS=		${RPCSRCS:R:S/$/.c/g}
HDRS=           ${RPCSRCS:R:S/$/.h/g}
CLEANFILES+=	${SRCS} ${HDRS}

NOMAN=		noman
NOPIC=

beforedepend:	${HDRS}
all:		${HDRS}

# new suffixes have to go afterwards, because bsd.lib.mk clears them
.include <bsd.lib.mk>
.include "${.CURDIR}/../Makefile.dep"
