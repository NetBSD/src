#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93
#	$Id: Makefile,v 1.6 1994/09/24 01:02:51 mycroft Exp $

PROG=	getty
SRCS=	main.c init.c subr.c
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
MAN5=	gettytab.0 ttys.0
MAN8=	getty.0

.include <bsd.prog.mk>
