#	$NetBSD: Makefile,v 1.8 1997/06/01 03:39:29 mrg Exp $

PROG=		bootxx
PROGSOURCE=	bootxx.c

.include "../Makefile.buildboot"

CPPFLAGS+=	-DBOOTXX
STRIPFLAG=

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${LD} -o ${.TARGET} -N -T ${RELOC} -e start ${OBJS} ${LIBSA} ${LIBKERN}
	@size ${.TARGET}

.include <bsd.prog.mk>
