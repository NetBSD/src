#	$NetBSD: Makefile,v 1.7 2011/03/06 17:08:16 bouyer Exp $

.include <bsd.own.mk>
PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8
MLINKS = rpc.rquotad.8 rquotad.8

CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBRPCSVC} ${LIBPROP}
LDADD=	-lrpcsvc -lprop

.PATH:  ${NETBSDSRCDIR}/usr.bin/quota
SRCS+=	getvfsquota.c
.PATH:  ${NETBSDSRCDIR}/sys/ufs/ufs
SRCS+=	quota2_prop.c quota1_subr.c


.include <bsd.prog.mk>
