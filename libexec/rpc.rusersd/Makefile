#	$NetBSD: Makefile,v 1.17 2000/01/23 09:58:13 veego Exp $

PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c
MAN =	rpc.rusersd.8
MLINKS=	rpc.rusersd.8 rusersd.8

DPADD+=	${LIBRPCSVC}
LDADD+=	-lrpcsvc

.if exists(/usr/X11R6)
XINCDIR=	/usr/X11R6/include
XLIBDIR=	/usr/X11R6/lib
.endif

#.if defined(XINCDIR) && exists($(XINCDIR)/X11/extensions/xidle.h)
#CPPFLAGS+=	-I$(XINCDIR) -DXIDLE
#LDADD+=		-L$(XLIBDIR) -lXext -lX11
#.endif

.include <bsd.prog.mk>

