#	$NetBSD: Makefile,v 1.9 2003/02/28 05:24:56 matt Exp $

PROG=		bootxx
NOMAN=		# defined
BINMODE=	444

CFLAGS=		-Os -mmultiple -Wall -msoft-float -ffreestanding
CPPFLAGS+=	-D_STANDALONE -DPPC_OEA
STRIPFLAG=
LINKFLAGS=	-x -N -Ttext 4000 -e _start
CLEANFILES+=	${PROG}.sym

${PROG}: ${OBJS}
	${LD} -o ${.TARGET}.sym ${LINKFLAGS} ${OBJS}
	@${SIZE} ${.TARGET}.sym
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.prog.mk>
