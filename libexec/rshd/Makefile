#	from: @(#)Makefile	5.6 (Berkeley) 9/27/90
#	$Id: Makefile,v 1.2 1993/08/01 05:38:59 mycroft Exp $

PROG=	rshd
SRCS=	rshd.c
MAN8=	rshd.0
DPADD=	${LIBUTIL}
LDADD=	-lutil
.PATH: ${.CURDIR}/../../usr.bin/rlogin

.include <bsd.prog.mk>
