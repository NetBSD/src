#	from: @(#)Makefile	5.3 (Berkeley) 12/4/90
#	$Id: Makefile,v 1.5 1993/08/09 04:42:36 mycroft Exp $

PROG=	date
SRCS=	date.c netdate.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
