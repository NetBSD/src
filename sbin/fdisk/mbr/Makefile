#	$NetBSD: Makefile,v 1.3.2.2 1999/05/02 21:29:41 perry Exp $

MAN=	mbr.8

BINDIR=	/usr/mdec
BINMODE=444

STRIPFLAG=

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF"
LDFLAGS+= -e start
.else
LDFLAGS+= -e _start -N
.endif

.if ${MACHINE} == "i386"
PROG=	mbr
SRCS=	mbr.S

${PROG}: ${OBJS}
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0x600 ${OBJS}
	${STRIPPROG} ${PROG}.tmp
	objcopy -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp
.endif

.include <bsd.prog.mk>
