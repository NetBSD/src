#	$NetBSD: Makefile.inc,v 1.3 1997/05/09 07:50:29 mycroft Exp $

BINDIR=		/usr/mdec
BINMODE=	${NONBINMODE}

STRIPFLAG=
LDFLAGS=	-nostdlib -static -Wl,-T0,-N
CPPFLAGS+=	-I- -I. -I..

beforeinstall:
	dd if=${PROG} of=${PROG}X bs=32 skip=1 count=16  2>/dev/null
	@mv -f ${PROG} ${PROG}XX
	@mv -f ${PROG}X ${PROG}

afterinstall:
	@mv -f ${PROG}XX ${PROG}

.s.o: ../LOADADDR
	${CPP} ${CPPFLAGS} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET}
	@if nm ${.TARGET} | grep -q "00000200 T end";	\
	 then						\
		true;					\
	 else						\
		nm -g ${.TARGET} | sort;		\
		rm -f ${.TARGET};			\
		false;					\
	 fi

.include "../LOADADDR"
