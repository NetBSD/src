#	$NetBSD: Makefile,v 1.16 2017/04/20 19:09:29 uwe Exp $

S!=	cd ${.CURDIR}/../../../..; pwd

PROG=		bootxx
NOMAN=		# defined
BINMODE=	444

LIBCRT0=	# nothing
LIBCRTI=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

CFLAGS=		-Os -Wall -ffreestanding
CFLAGS+=	${${ACTIVE_CC} == "gcc":? -msoft-float :}
CFLAGS+=	${CFLAGS_UNWIND.${ACTIVE_CC}}
CPPFLAGS+=	-D_STANDALONE -DPPC_OEA -I${.OBJDIR} -I${S}
STRIPFLAG=
LINKFLAGS=	-x -N -Ttext 4000 -e _start
LINKFLAGS+=	${LINKFLAGS_UNWIND}
CLEANFILES+=	${PROG}.sym

${PROG}: ${OBJS}
	${_MKTARGET_LINK}
	${LD} -o ${.TARGET}.sym ${LINKFLAGS} ${OBJS}
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
