#	$NetBSD: Makefile,v 1.15.2.1 2004/11/12 05:51:54 jmc Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

WARNS=	2
PROG=	rsh
SRCS=	rsh.c getport.c
#CPPFLAGS+=-DKERBEROS -DCRYPT
#SRCS=	rsh.c krcmd.c kcmd.c des_rw.c
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes
.PATH:	${NETBSDSRCDIR}/usr.bin/rlogin

.include <bsd.prog.mk>
