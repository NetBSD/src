#	$NetBSD: Makefile,v 1.4.12.3 2002/06/20 03:39:40 nathanw Exp $

PROG=		bootxx
NOMAN=		# defined
BINMODE=	444

CFLAGS=		-Os -mmultiple -Wall -msoft-float
CPPFLAGS+=	-D_STANDALONE
STRIPFLAG=
LINKFLAGS=	-x -N -Ttext 4000 -e _start
CLEANFILES+=	${PROG}.sym

${PROG}: ${OBJS}
	${LD} -o ${.TARGET}.sym ${LINKFLAGS} ${OBJS}
	@${SIZE} ${.TARGET}.sym
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.prog.mk>
