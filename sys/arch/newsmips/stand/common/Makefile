#	$NetBSD: Makefile,v 1.2 2000/01/23 17:04:06 mycroft Exp $

S= ${.CURDIR}/../../../..

OBJS= romcalls.o

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
.NOPATH: machine mips
CLEANFILES+= machine mips
.endif

realall: ${OBJS} ${KERNLIB} ${ZLIB} ${SALIB}
#	ln -s ${KERNLIB}
#	ln -s ${ZLIB}
#	ln -s ${SALIB}

.include <bsd.prog.mk>
