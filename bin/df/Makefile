#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.5 1993/08/06 17:05:08 mycroft Exp $

PROG=	df
CFLAGS+=-DCOMPAT_43
LDADD=	-lutil
DPADD=	${LIBUTIL}
BINGRP=	operator
BINMODE=2555

.include <bsd.prog.mk>
