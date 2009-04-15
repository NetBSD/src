#	$NetBSD: Makefile,v 1.11 2009/04/15 17:15:13 tsutsui Exp $

WARNS?= 1
S!= cd ${.CURDIR}/../../../..; pwd

OBJS= romcalls.o

CFLAGS= -Os -ffreestanding -mmemcpy -mno-abicalls -msoft-float -G 0
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

CPPFLAGS+= -D_STANDALONE #-DBOOT_DEBUG
CPPFLAGS+= -DHAVE_CHANGEDISK_HOOK
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

realall: ${OBJS} ${KERNLIB} ${ZLIB} ${SALIB}
#	ln -s ${KERNLIB}
#	ln -s ${ZLIB}
#	ln -s ${SALIB}

.include <bsd.klinks.mk>
.include <bsd.prog.mk>
