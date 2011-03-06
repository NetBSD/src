#	$NetBSD: Makefile,v 1.8 2011/03/06 20:47:59 christos Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS ?= 4
.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c getvfsquota.c

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBRPCSVC} ${LIBPROP}
LDADD=	-lrpcsvc -lprop

.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS+=	quota2_prop.c quota2_subr.c quota1_subr.c
.include <bsd.prog.mk>
