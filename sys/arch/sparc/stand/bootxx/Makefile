#	$NetBSD: Makefile,v 1.15 2004/07/28 01:05:35 simonb Exp $

PROG=		bootxx
PROGSOURCE=	bootxx.c

.include "../Makefile.buildboot"

CPPFLAGS+=	-DBOOTXX -DLIBSA_NO_CLOSE
STRIPFLAG=
LINKFLAGS=	-N -Ttext ${RELOC_BOOTXX} -e start
CLEANFILES+=	${PROG}.sym

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${LD} -o ${.TARGET}.sym ${LINKFLAGS} ${OBJS} ${LIBSA} ${LIBKERN}
	@${SIZE} ${.TARGET}.sym
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.prog.mk>
