#	$NetBSD: Makefile,v 1.6.2.1 1999/04/22 14:51:12 perry Exp $

PROG=	bootxx
SRCS=	bootxx.c bsdstart.s filesystem.c
MKMAN=	no

STRIPFLAG=
BINDIR=	/usr/mdec
BINMODE=${NONBINMODE}

LIBCRT0=

DEFS=	-D_STANDALONE
INCL=	-I. -I${S_LIBTOS} -I${S_MACHSA} -I${S_KERN} -I${S_SA} -I${S}

CPPFLAGS+=	${DEFS} ${INCL}
CFLAGS+=	-fomit-frame-pointer -Wall
LDFLAGS=	-static -nostdlib -Wl,-N,-T${LOADADDR}

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
	@len=`size ${PROG} | awk 'NR==2 { print $$1+$$2 }'`;	\
	 if [ $$len -gt 6656 ]; then				\
		size ${PROG};					\
		false;						\
	 fi;							\
	 dd if=${PROG} of=${PROG}XX bs=1 skip=32		\
			count=$$len  2>/dev/null
	@cat ${PROG}XX /dev/zero | dd of=${PROG}X		\
			bs=1 count=6656  2>/dev/null
	@mv -f ${PROG} ${PROG}XX
	@mv -f ${PROG}X ${PROG};

afterinstall:
	@mv -f ${PROG}XX ${PROG}

.s.o:
	${CPP} ${CPPFLAGS} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET}

${PROG}: ../xxboot/LOADADDR

.PATH: ${S_LIBTOS}

.include "../xxboot/LOADADDR"
.include <bsd.prog.mk>
