#	$NetBSD: Makefile,v 1.9.2.1 2000/02/22 23:06:31 he Exp $

PROG=		bootxx
PROGSOURCE=	bootxx.c

.include "../Makefile.buildboot"

CPPFLAGS+=	-DBOOTXX
STRIPFLAG=
LINKFLAGS=	-N -Ttext ${RELOC_BOOTXX} -e start

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${LD} -o ${.TARGET} ${LINKFLAGS} ${OBJS} ${LIBSA} ${LIBKERN}
	@size ${.TARGET}

.include <bsd.prog.mk>
