#	$Id: Makefile,v 1.7 1993/11/21 20:19:15 brezak Exp $

PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c
MAN8 =	rpc.rusersd.0

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

.if exists(/usr/include/X11/extensions/xidle.h)
CFLAGS+= -DXIDLE
LDADD+= -L/usr/X386/lib -lXext -lX11
.endif


.include <bsd.prog.mk>

