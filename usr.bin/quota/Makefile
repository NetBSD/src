#	$NetBSD: Makefile,v 1.6.64.5 2011/02/14 20:35:24 bouyer Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c getvfsquota.c

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBRPCSVC} ${LIBPROP}
LDADD=	-lrpcsvc -lprop

.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS+=	quota2_prop.c quota2_subr.c quota1_subr.c
.include <bsd.prog.mk>
