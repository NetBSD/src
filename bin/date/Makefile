#	from: @(#)Makefile	8.1 (Berkeley) 5/31/93
#	$Id: Makefile,v 1.6 1994/09/22 09:24:46 mycroft Exp $

PROG=	date
SRCS=	date.c netdate.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
