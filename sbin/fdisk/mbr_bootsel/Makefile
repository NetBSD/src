#	$NetBSD: Makefile,v 1.10 2002/09/18 06:38:51 lukem Exp $

NOMAN=	# defined
STRIPFLAG=

.include <bsd.own.mk>

BINDIR=	/usr/mdec
BINMODE=444

PROG=	mbr_bootsel
SRCS=	mbr_bootsel.S

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
