#	$NetBSD: Makefile,v 1.16 2005/12/11 12:19:08 christos Exp $

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
