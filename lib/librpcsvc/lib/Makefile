#	$Id: Makefile,v 1.2 1993/10/08 05:44:44 cgd Exp $

RPCDIR=		${.CURDIR}/../src
.PATH:		${RPCDIR}

.include "${.CURDIR}/../Makefile.src"

LIB=		rpcsvc

CFLAGS+=	-I/usr/include/rpcsvc
SRCS=		${RPCSRCS:R:S/$/.c/g}
CLEANFILES+=	${SRCS}

NOMAN=		noman
NOPIC=

beforedepend:	bootparam_prot.h yp.h
CLEANFILES+=	bootparam_prot.h yp.h

# BECAUSE:
bootparam_prot.o: bootparam_prot.h
yp.o: yp.h

# new suffixes have to go afterwards, because bsd.lib.mk clears them
.include <bsd.lib.mk>
.include "${.CURDIR}/../Makefile.dep"
