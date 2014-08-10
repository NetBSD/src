#	$NetBSD: Makefile,v 1.13.2.1 2014/08/10 06:54:02 tls Exp $

S!=	cd ${.CURDIR}/../../../..; pwd

PROG=		bootxx
NOMAN=		# defined
BINMODE=	444

LIBCRT0=	# nothing
LIBCRTI=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

CFLAGS=		-Os -Wall -msoft-float -ffreestanding
CPPFLAGS+=	-D_STANDALONE -DPPC_OEA -I${.OBJDIR} -I${S}
STRIPFLAG=
LINKFLAGS=	-x -N -Ttext 4000 -e _start
CLEANFILES+=	${PROG}.sym

${PROG}: ${OBJS}
	${_MKTARGET_LINK}
	${LD} -o ${.TARGET}.sym ${LINKFLAGS} ${OBJS}
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
