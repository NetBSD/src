#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91
#	$Id: Makefile,v 1.3 1993/09/25 13:22:28 pk Exp $

PROG=	iostat
CFLAGS+=-I${DESTDIR}/sys -I${.CURDIR}/../../usr.bin/vmstat
MAN8=	iostat.0
DPADD=	${LIBUTIL}
LDADD=	-lutil
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
