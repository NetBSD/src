#	$NetBSD: Makefile,v 1.1.1.1 1996/02/29 11:35:05 leo Exp $

PROG=	bootxx
SRCS=	bootxx.c bsdstart.s filesystem.c
NOMAN=

STRIP=
BINDIR=	/usr/mdec
BINMODE=${NONBINMODE}

DPADD=	../libsa/libsa.a
LDADD=	../libsa/libsa.a

DEFS=	-DSTANDALONE
INCL=	-I. -I${S_LIBTOS} -I${S_MACHSA} -I${S_KERN} -I${S_SA} -I${S}

CPPFLAGS+=	${DEFS} ${INCL}
CFLAGS+=	-fomit-frame-pointer -Wall
LDFLAGS=	-static -nostdlib -Wl,-N,-T${LOADADDR}

# logically src/sys
S=../../../..
S_SA=${S}/lib/libsa
S_KERN=${S}/lib/libkern
S_MACHSA=${S}/arch/atari/stand/libsa
S_LIBTOS=${S}/arch/atari/stand/tostools/libtos

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
