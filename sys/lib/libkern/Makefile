#	$NetBSD: Makefile,v 1.28 1996/08/08 15:06:42 mycroft Exp $

LIB=	kern
NOPIC=

CPPFLAGS=	-I${.CURDIR}/arch/${MACHINE_ARCH} ${KERNCPPFLAGS}

.if exists (${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc)
.PATH:	${.CURDIR}/arch/${MACHINE_ARCH}
.include "${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.endif

.if (${MACHINE_ARCH} != "alpha")
# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c  ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
.endif

# Other stuff
SRCS+=	

# Files to clean up
CLEANFILES+= lib${LIB}.o lib${LIB}.po

# mcount cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po

install:

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`

POBJS+=	${OBJS:.o=.po} mcount.po
lib${LIB}.po:: ${POBJS}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}.po
	@${LD} -r -o lib${LIB}.po `lorder ${POBJS} | tsort`
