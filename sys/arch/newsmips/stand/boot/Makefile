#	$NetBSD: Makefile,v 1.1 1999/07/08 11:48:05 tsubai Exp $

.PATH: ../bootxx

S= ${.CURDIR}/../../../..

PROG= boot
SRCS= locore.S romcalls.S boot.c devopen.c
MKMAN= no
STRIPFLAG=
BINMODE= 444

OBJCOPY?= objcopy
SIZE?= size
STRIP?= strip

CLEANFILES+= ${PROG}.elf ${PROG}.tmp
CLEANFILES+= ${.OBJDIR}/machine ${.OBJDIR}/mips

LDFLAGS= -x -N -Ttext a0700000
CFLAGS= -Os -G 0 -mno-abicalls -Wall

CPPFLAGS+= -D_STANDALONE #-DBOOT_DEBUG
CPPFLAGS+= -I. -I../../../..

AFLAGS= -D_LOCORE

### find out what to use for libkern
#KERNDST= ${.OBJDIR}/../lib/kern
KERN_AS= library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}

### find out what to use for libz
#ZDST= ${.OBJDIR}/../lib/z
Z_AS= library
.include "${S}/lib/libz/Makefile.inc"
LIBZ= ${ZLIB}

### find out what to use for libsa
#SADST= ${.OBJDIR}/../lib/sa
SA_AS= library
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_USE_LOADFILE=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA= ${SALIB}

.BEGIN:
	@[ -h machine ] || ln -s ${S}/arch/${MACHINE}/include machine
	@[ -h mips ] || ln -s ${S}/arch/mips/include mips

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} ${LDFLAGS} -o ${PROG} ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	-${SIZE} ${PROG}
	cp ${PROG} ${PROG}.elf
	${STRIP} ${PROG}
	tail -c +177 ${PROG} > ${PROG}.tmp
	mv ${PROG}.tmp ${PROG}
#	${OBJCOPY} -O binary ${PROG}.elf ${PROG}	# XXX

.include <bsd.prog.mk>
