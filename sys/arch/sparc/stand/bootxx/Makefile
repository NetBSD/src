#	$NetBSD: Makefile,v 1.9 1999/03/05 11:15:53 pk Exp $

PROG=		bootxx
PROGSOURCE=	bootxx.c

.include "../Makefile.buildboot"

CPPFLAGS+=	-DBOOTXX
STRIPFLAG=
LINKFLAGS=	-N -Ttext ${RELOC} -e start

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${LD} -o ${.TARGET} ${LINKFLAGS} ${OBJS} ${LIBSA} ${LIBKERN}
	@size ${.TARGET}

.include <bsd.prog.mk>
