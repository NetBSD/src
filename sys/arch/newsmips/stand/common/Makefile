#	$NetBSD: Makefile,v 1.8 2003/03/30 08:52:23 tsutsui Exp $

WARNS?= 1
S= ${.CURDIR}/../../../..

OBJS= romcalls.o

CFLAGS= -Os -ffreestanding -mmemcpy -mno-abicalls -msoft-float -G 0

CPPFLAGS+= -D_STANDALONE #-DBOOT_DEBUG
CPPFLAGS+= -DHAVE_CHANGEDISK_HOOK
CPPFLAGS+= -DLIBSA_USE_MEMSET -DLIBSA_USE_MEMCPY
CPPFLAGS+= -I. -I${S}
CPPFLAGS+= -D__daddr_t=int32_t

AFLAGS= -D_LOCORE -mno-abicalls

### find out what to use for libkern
.include "${S}/lib/libkern/Makefile.inc"

### find out what to use for libz
.include "${S}/lib/libz/Makefile.inc"

### find out what to use for libsa
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h mips ] || ln -s ${S}/arch/mips/include mips
.NOPATH: machine mips
.endif
CLEANFILES+= machine mips

realall: ${OBJS} ${KERNLIB} ${ZLIB} ${SALIB}
#	ln -s ${KERNLIB}
#	ln -s ${ZLIB}
#	ln -s ${SALIB}

.include <bsd.prog.mk>
