#	$NetBSD: Makefile,v 1.2 1999/03/17 14:52:39 minoura Exp $

PROG=	aout2hux
MKMAN=	no			# See share/man/man1/man1.x68k/aout2hux.1

# only needed during build
proginstall::

# Override these rules for cross-compilation
.c.o:
	${HOST_COMPILE.c} ${.IMPSRC}

${PROG}: ${OBJS}
	${HOST_LINK.c} -o ${.TARGET} ${OBJS}

.include <bsd.prog.mk>

${OBJS}: aout68k.h hux.h type_local.h
