#	$NetBSD: Makefile,v 1.18 2002/11/04 22:03:38 christos Exp $

.include <bsd.own.mk>

PROG =	rpc.rusersd
SRCS =	rusersd.c rusers_proc.c utmpentry.c
MAN =	rpc.rusersd.8
MLINKS=	rpc.rusersd.8 rusersd.8

.PATH.c: ${NETBSDSRCDIR}/usr.bin/who

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP


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

