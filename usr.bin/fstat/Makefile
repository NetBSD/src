#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.7 1994/06/13 22:12:07 mycroft Exp $

PROG=	fstat
CFLAGS+=-I${DESTDIR}/sys
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
