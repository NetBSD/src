#	$Id: Makefile,v 1.6 1993/11/21 18:56:33 brezak Exp $
PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c rusersxdr.c
MAN8 =	rpc.rusersd.0

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

.if exists(/usr/include/X11/extensions/xidle.h)
CFLAGS+= -DXIDLE
LDADD+= -L/usr/X386/lib -lXext -lX11
.endif


.include <bsd.prog.mk>

