#	$NetBSD: Makefile.inc,v 1.1.1.1 1996/02/29 11:36:49 leo Exp $

BINDIR=		/usr/mdec
BINMODE=	${NONBINMODE}

STRIP=
LDFLAGS=	-nostdlib -static -Wl,-T0,-N
CPPFLAGS+=	-I- -I. -I..

depend:
	@rm -f .depend
	${CPP} ${CPPFLAGS} -M ${SRCS} >.depend

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
