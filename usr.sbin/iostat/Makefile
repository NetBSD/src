#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.4 1994/01/28 00:51:04 cgd Exp $

PROG=	iostat
CFLAGS+=-I${DESTDIR}/sys -I${.CURDIR}/../../usr.bin/vmstat
MAN8=	iostat.0
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
