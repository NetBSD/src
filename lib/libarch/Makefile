#	$NetBSD: Makefile,v 1.16 2001/07/28 19:17:11 bjh21 Exp $

ARCHDIRS= alpha arm32 i386 m68k

.for arch in ${ARCHDIRS}
.include "${.CURDIR}/${arch:C/mipse[bl]/mips/}/Makefile.inc"
.PATH:	${.CURDIR}/${arch:C/mipse[bl]/mips/}
.endfor

OBJS+= ${ASM}
POBJS+=	${ASM:.o=.po}
CLEANFILES+= ${ASM} ${POBJS}

# Build the library if SRCS is now defined ...
.if defined(SRCS)
LIB=    ${MACHINE_ARCH:C/mipse[bl]/mips/:C/arm.*/arm32/}
SHLIB_VERSION_FILE= ${.CURDIR}/${LIB}/shlib_version

.include <bsd.lib.mk>
.else
# ... otherwise just build the manpages
.include <bsd.man.mk>
.endif
