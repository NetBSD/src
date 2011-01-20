#	$NetBSD: Makefile,v 1.6.64.1 2011/01/20 14:25:05 bouyer Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c

CPPFLAGS+=-I${NETBSDSRCDIR}/sys
DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc -lprop

.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS+=	quota2_prop.c
.include <bsd.prog.mk>
