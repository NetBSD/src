#	$NetBSD: Makefile,v 1.11 1999/08/21 14:18:36 sommerfeld Exp $

PROG=		bootxx
PROGSOURCE=	bootxx.c

.include "../Makefile.buildboot"

CPPFLAGS+=	-DBOOTXX -DLIBSA_NO_CLOSE
STRIPFLAG=
LINKFLAGS=	-N -Ttext ${RELOC} -e start

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${LD} -o ${.TARGET} ${LINKFLAGS} ${OBJS} ${LIBSA} ${LIBKERN}
	@${SIZE} ${.TARGET}

.include <bsd.prog.mk>
