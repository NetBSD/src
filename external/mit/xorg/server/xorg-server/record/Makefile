#	$NetBSD: Makefile,v 1.3 2020/09/13 23:01:02 mrg Exp $

LIBISPRIVATE=	yes

.include "../Makefile.serverlib"

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg

CPPFLAGS+=      ${X11FLAGS.DIX} ${X11INCS.DIX}

LIB=	record
SRCS=	${SRCS.record}

.include "Makefile.record"
