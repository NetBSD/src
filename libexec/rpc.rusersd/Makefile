#	$Id: Makefile,v 1.8 1994/01/28 00:31:32 cgd Exp $

PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c
MAN8 =	rpc.rusersd.0

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.if exists(/usr/include/X11/extensions/xidle.h)
CFLAGS+= -DXIDLE
LDADD+= -L/usr/X386/lib -lXext -lX11
.endif


.include <bsd.prog.mk>

