#	$NetBSD: Makefile,v 1.10 2011/03/24 17:05:46 bouyer Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS ?= 4
.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c getvfsquota.c

CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC} ${LIBPROP}
LDADD=	-lquota -lrpcsvc -lprop

.include <bsd.prog.mk>
