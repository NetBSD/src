#	$NetBSD: Makefile,v 1.23 2004/09/05 08:38:23 manu Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=		rcp
SRCS=		rcp.c util.c

# XXX Kerberos support broken right now.
# XXX Unsupported Kerberos options were removed from man page
# XXX Don't forget to update the man page if you fix Kerberos
USE_KERBEROS4=	no

.if (${USE_KERBEROS4} != "no")
RLOGIN= 	${NETBSDSRCDIR}/usr.bin/rlogin
.PATH:		${RLOGIN}

SRCS+=		krcmd.c kcmd.c
CPPFLAGS+=	-DKERBEROS -DCRYPT -I${RLOGIN}
LDADD+= 	-lkrb -ldes
DPADD+= 	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
