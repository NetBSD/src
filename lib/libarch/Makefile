#	$NetBSD: Makefile,v 1.13 1999/08/31 13:50:47 simonb Exp $

ARCHDIRS= arm32 i386 m68k

.for arch in ${ARCHDIRS}
.include "${.CURDIR}/${arch:C/mipse[bl]/mips/}/Makefile.inc"
.PATH:	${.CURDIR}/${arch:C/mipse[bl]/mips/}
.endfor

OBJS+= ${ASM}
POBJS+=	${ASM:.o=.po}
CLEANFILES+= ${ASM} ${POBJS}

# Build the library if SRCS is now defined ...
.if defined(SRCS)
LIB=    ${MACHINE_ARCH:C/mipse[bl]/mips/}
SHLIB_VERSION_FILE= ${.CURDIR}/${MACHINE_ARCH:C/mipse[bl]/mips/}/shlib_version

.include <bsd.lib.mk>
.else
# ... otherwise just build the manpages
.include <bsd.man.mk>
.endif
