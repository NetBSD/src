#	$NetBSD: Makefile.inc,v 1.5.4.2 2002/04/17 00:02:46 nathanw Exp $

BINDIR=		/usr/mdec
BINMODE=	${NONBINMODE}

STRIPFLAG=
LDFLAGS=	-nostdlib -static -Wl,-x,-Ttext,0,-N
CPPFLAGS+=	-I- -I. -I..

beforeinstall:
	${OBJCOPY} -O binary ${PROG} ${PROG}X
	@mv -f ${PROG} ${PROG}XX
	@mv -f ${PROG}X ${PROG}

afterinstall:
	@mv -f ${PROG}XX ${PROG}

.s.o: ../LOADADDR
	${CPP} ${CPPFLAGS} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET}
	@if nm ${.TARGET} | grep -q "00000200 t end";	\
	 then						\
		true;					\
	 else						\
		nm ${.TARGET} | grep -q " end";		\
		rm -f ${.TARGET};			\
		false;					\
	 fi

.include "../LOADADDR"
