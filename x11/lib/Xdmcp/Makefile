#	$NetBSD: Makefile,v 1.5 2005/01/21 05:15:36 rtr Exp $

.include <bsd.own.mk>

LIB=	Xdmcp
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	A8Eq.c AA16.c AA32.c AA8.c Alloc.c AofA8.c CA8.c CmpKey.c DA16.c \
	DA32.c DA8.c DAofA8.c DecKey.c Fill.c Flush.c GenKey.c \
	IncKey.c RA16.c RA32.c RA8.c RAofA8.c RC16.c RC32.c RC8.c \
	RHead.c RR.c RaA16.c RaA32.c RaA8.c RaAoA8.c Unwrap.c WA16.c \
	WA32.c WA8.c WAofA8.c WC16.c WC32.c WC8.c Whead.c Wrap.c Wraphelp.c

INCS=	Xdmcp.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=${X11FLAGS.THREADS}
CPPFLAGS+=-DHASXDMAUTH

CPPFLAGS.Fill.c=	${X11FLAGS.CONNECTION}
CPPFLAGS.Flush.c=	${X11FLAGS.CONNECTION}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
