#	$NetBSD: Makefile,v 1.6.64.2 2011/01/21 16:58:06 bouyer Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBRPCSVC} ${LIBPROP}
LDADD=	-lrpcsvc -lprop

.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS+=	quota2_prop.c
.include <bsd.prog.mk>
