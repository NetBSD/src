#	$NetBSD: Makefile,v 1.16 2004/09/05 08:38:23 manu Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=	rsh
WARNS=	2
# XXX Kerberos support broken right now.
# XXX Unsupported Kerberos options were removed from man page
# XXX Don't forget to update the man page if you fix Kerberos
#CPPFLAGS+=-DKERBEROS -DCRYPT
#SRCS=	rsh.c krcmd.c kcmd.c des_rw.c
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes
.PATH:	${NETBSDSRCDIR}/usr.bin/rlogin

.include <bsd.prog.mk>
