#	$NetBSD: Makefile,v 1.21 2019/01/27 04:59:12 dholland Exp $

NOLIBCSANITIZER=
NOSANITIZER=
NOPIE=

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
