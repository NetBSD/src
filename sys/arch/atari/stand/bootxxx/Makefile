#	$NetBSD: Makefile,v 1.1.1.1 2001/10/13 20:06:57 leo Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

PROG=	boot
#
# Warning: bootxxx.c should be the first source!
#
SRCS=	bootxxx.c aout.c bsdstart.s elf.c filesystem.c sysinfo.c
MKMAN=	no

STRIPFLAG=
BINDIR=	/
BINMODE=${NONBINMODE}

LIBCRT0=

DEFS=	-D_STANDALONE  -DHEAP_VARIABLE
INCL=	-I. -I${S_LIBTOS} -I${S_MACHSA} -I${S_KERN} -I${S_SA} -I${S}

CPPFLAGS+=	${DEFS} ${INCL}
CFLAGS+=	-fomit-frame-pointer -Wall -fno-function-cse -fstrength-reduce
.if ${OBJECT_FMT} == "ELF"
LDFLAGS=	-static -nostdlib -Wl,-x,-N,-e,bootxxx,-Ttext,${LOADADDR3}
.else
LDFLAGS=	-static -nostdlib -Wl,-N,-Ttext,${LOADADDR3}
.endif

# logically src/sys
S=${.CURDIR}/../../../..
S_SA=${S}/lib/libsa
S_KERN=${S}/lib/libkern
S_MACHSA=${S}/arch/atari/stand/libsa
S_LIBTOS=${S}/arch/atari/stand/tostools/libtos

.if exists(${S_MACHSA}/libsa.a)
LIBSA=${S_MACHSA}/libsa.a
.else
.	if exists(${S_MACHSA}/obj/libsa.a)
LIBSA=${S_MACHSA}/obj/libsa.a
.	else
LIBSA=${S_MACHSA}/obj.${MACHINE}/libsa.a
.	endif
.endif

DPADD=	${LIBSA}
LDADD=	${LIBSA}

beforeinstall:
	${OBJCOPY} -O binary ${PROG} ${PROG}X
	@mv -f ${PROG} ${PROG}XX
	@mv -f ${PROG}X ${PROG};

afterinstall:
	@mv -f ${PROG}XX ${PROG}

.s.o:
	${CPP} ${CPPFLAGS} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET}

.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -c ${.IMPSRC} -o ${.TARGET}

${PROG}:
	${CC} ${LDFLAGS} -o ${.TARGET} ${OBJS} ${LDADD}

${PROG}: ../xxboot/LOADADDR

.PATH: ${S_LIBTOS}

.include "../xxboot/LOADADDR"
.include <bsd.prog.mk>
