#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.2 1998/03/30 04:15:37 mrg Exp $

PROG=	g77
SRCS=	gcc.c g77spec.c version.c choose-temp.c pexecute.c obstack.c
BINDIR=	/usr/bin

LINKS+=	${BINDIR}/g77 ${BINDIR}/c77

MAN=	g77.1
MLINKS=	g77.1 f77.1

.include <bsd.prog.mk>

.PATH: $(DIST)/f $(DIST)
CPPFLAGS+= -I$(DIST)/f -I$(DIST)
