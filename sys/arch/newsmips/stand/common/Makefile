#	$NetBSD: Makefile,v 1.1 1999/12/17 07:40:11 tsubai Exp $

S= ${.CURDIR}/../../../..

OBJS= romcalls.o

CLEANFILES+= machine mips

CFLAGS= -Os -G 0 -mno-abicalls -Wall

CPPFLAGS+= -D_STANDALONE #-DBOOT_DEBUG
CPPFLAGS+= -I. -I${S}

AFLAGS= -D_LOCORE

### find out what to use for libkern
.include "${S}/lib/libkern/Makefile.inc"

### find out what to use for libz
.include "${S}/lib/libz/Makefile.inc"

### find out what to use for libsa
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"

.if !make(obj) && !make(clean)
.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h mips ] || ln -s ${S}/arch/mips/include mips
.endif

all: ${OBJS} ${KERNLIB} ${ZLIB} ${SALIB}
#	ln -s ${KERNLIB}
#	ln -s ${ZLIB}
#	ln -s ${SALIB}

.include <bsd.prog.mk>
