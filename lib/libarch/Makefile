#	$NetBSD: Makefile,v 1.12 1999/06/10 00:49:51 simonb Exp $

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

.include <bsd.lib.mk>
.else
# ... otherwise just build the manpages
.include <bsd.man.mk>
.endif
