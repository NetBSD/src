#	$NetBSD: Makefile,v 1.15 2002/09/19 11:31:22 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=	rsh
WARNS=	2
#CPPFLAGS+=-DKERBEROS -DCRYPT
#SRCS=	rsh.c krcmd.c kcmd.c des_rw.c
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes
.PATH:	${NETBSDSRCDIR}/usr.bin/rlogin

.include <bsd.prog.mk>
