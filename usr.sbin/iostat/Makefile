#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.2 1993/07/30 22:12:53 mycroft Exp $

PROG=	iostat
CFLAGS+=-I/sys -I${.CURDIR}/../../usr.bin/vmstat
MAN8=	iostat.0
DPADD=	${LIBUTIL}
LDADD=	-lutil
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
