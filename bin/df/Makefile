#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.6 1993/08/09 04:42:53 mycroft Exp $

PROG=	df
CFLAGS+=-DCOMPAT_43
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
BINGRP=	operator
BINMODE=2555

.include <bsd.prog.mk>
