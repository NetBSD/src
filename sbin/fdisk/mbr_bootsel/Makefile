#	$NetBSD: Makefile,v 1.1.2.2 1999/04/18 00:18:50 fvdl Exp $

BINDIR=	/usr/mdec
BINMODE=444

STRIPFLAG=

PROG=	mbr_bootsel
SRCS=	mbr_bootsel.S
MKMAN=	no

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF"
LDFLAGS+= -e start
.else
LDFLAGS+= -e _start -N
.endif

${PROG}: ${OBJS}
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0x600 ${OBJS}
	${STRIPPROG} ${PROG}.tmp
	objcopy -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>
