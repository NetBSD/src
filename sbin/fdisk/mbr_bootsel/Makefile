#	$NetBSD: Makefile,v 1.9 2001/12/12 01:48:43 tv Exp $

BINDIR=	/usr/mdec
BINMODE=444

STRIPFLAG=

PROG=	mbr_bootsel
SRCS=	mbr_bootsel.S
NOMAN=	# defined

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF"
LDFLAGS+= -e start
.else
LDFLAGS+= -e _start -N
.endif

${PROG}: ${OBJS}
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0x600 ${OBJS}
	${STRIP} ${PROG}.tmp
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>
