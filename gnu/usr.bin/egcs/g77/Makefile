#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.1 1998/03/30 03:00:59 mrg Exp $

.if	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "mips")
PROG=	g77
SRCS=	gcc.c g77spec.c version.c choose-temp.c pexecute.c obstack.c
BINDIR=	/usr/bin

LINKS+=	${BINDIR}/g77 ${BINDIR}/c77
.endif

MAN=	g77.1
MLINKS=	g77.1 f77.1

.include <bsd.prog.mk>

.PATH: $(DIST)/f $(DIST)
CPPFLAGS+= -I$(DIST)/f -I$(DIST)
