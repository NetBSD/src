#	$Id: Makefile,v 1.4 1994/01/28 01:01:33 cgd Exp $

PROG=	swapinfo
SRCS=	swapinfo.c devname.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
