#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.5.64.2 2011/02/08 22:34:18 bouyer Exp $

.include <bsd.own.mk>
PROG=	edquota
SRCS=	edquota.c
MAN=	edquota.8

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBPROP}
LDADD=	-lprop

.PATH:	${NETBSDSRCDIR}/usr.bin/quota
SRCS+=	getvfsquota.c printquota.c
.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs 
SRCS+=	quota2_prop.c quota1_subr.c

.include <bsd.prog.mk>
