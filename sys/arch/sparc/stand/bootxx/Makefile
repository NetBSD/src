#	$NetBSD: Makefile,v 1.10 1999/04/28 13:27:18 christos Exp $

PROG=		bootxx
PROGSOURCE=	bootxx.c

.include "../Makefile.buildboot"

CPPFLAGS+=	-DBOOTXX -DLIBSA_NO_CLOSE
STRIPFLAG=
LINKFLAGS=	-N -Ttext ${RELOC} -e start

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${LD} -o ${.TARGET} ${LINKFLAGS} ${OBJS} ${LIBSA} ${LIBKERN}
	@size ${.TARGET}

.include <bsd.prog.mk>
