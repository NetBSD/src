#	from: @(#)Makefile	5.12 (Berkeley) 1/21/91
#	$Id: Makefile,v 1.4 1994/03/09 04:59:55 cgd Exp $

PROG=	getty
SRCS=	main.c init.c subr.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN5=	gettytab.0 ttys.0
MAN8=	getty.0

.include <bsd.prog.mk>
