#	from: @(#)Makefile	5.9 (Berkeley) 9/27/90
#	$Id: Makefile,v 1.2 1993/08/01 05:39:03 mycroft Exp $

PROG=	rlogind
SRCS=	rlogind.c
MAN8=	rlogind.0
DPADD=	${LIBUTIL}
LDADD=	-lutil
.PATH: ${.CURDIR}/../../usr.bin/rlogin

.include <bsd.prog.mk>
