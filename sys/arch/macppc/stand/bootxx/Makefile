#	$NetBSD: Makefile,v 1.10 2004/04/09 04:01:42 matt Exp $

PROG=		bootxx
NOMAN=		# defined
BINMODE=	444

CFLAGS=		-Os -mmultiple -Wall -msoft-float -ffreestanding
CPPFLAGS+=	-D_STANDALONE -DPPC_OEA
STRIPFLAG=
LINKFLAGS=	-x -N -Ttext 4000 -e _start
CLEANFILES+=	${PROG}.sym

${PROG}: ${OBJS}
	${_MKTARGET_LINK}
	${LD} -o ${.TARGET}.sym ${LINKFLAGS} ${OBJS}
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.prog.mk>
