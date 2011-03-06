#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.2 2011/03/06 17:08:43 bouyer Exp $

.include <bsd.own.mk>
PROG=	quotactl
SRCS=	quotactl.c
MAN=	quotactl.8

CPPFLAGS+=-I${NETBSDSRCDIR}/sys 
DPADD=	${LIBPROP}
LDADD=	-lprop

.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS+=	quota2_prop.c

.include <bsd.prog.mk>
