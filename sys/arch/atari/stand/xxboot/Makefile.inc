#	$NetBSD: Makefile.inc,v 1.3.36.1 2001/09/13 01:13:23 thorpej Exp $

BINDIR=		/usr/mdec
BINMODE=	${NONBINMODE}

STRIPFLAG=
LDFLAGS=	-nostdlib -static -Wl,-x,-Ttext,0,-N
CPPFLAGS+=	-I- -I. -I..

beforeinstall:
	objcopy -O binary ${PROG} ${PROG}X
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
