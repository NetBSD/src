#	$NetBSD: Makefile,v 1.19 2020/09/06 07:20:29 mrg Exp $

PROG=		bootxx
PROGSOURCE=	bootxx.c

.include "../Makefile.buildboot"

CPPFLAGS+=	-DBOOTXX -DLIBSA_NO_CLOSE
STRIPFLAG=
LINKFLAGS=	-N -Ttext ${RELOC_BOOTXX} -e start
CLEANFILES+=	${PROG}.sym

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

# XXX SHOULD NOT NEED TO DEFINE THESE!
LIBCRT0=
LIBCRTI=
LIBC=
LIBCRTBEGIN=
LIBCRTEND=

${PROG}: ${OBJS} ${LIBSA} ${LIBKERN}
	${LD} -o ${.TARGET}.sym ${LINKFLAGS} ${OBJS} ${LIBSA} ${LIBKERN}
	@${SIZE} ${.TARGET}.sym
	${OBJCOPY} -O binary ${.TARGET}.sym ${.TARGET}

.include <bsd.prog.mk>
