#	$NetBSD: Makefile,v 1.6 2001/01/14 09:24:30 christos Exp $

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

${PROG}: .NOTMAIN ${SRCS:S/.S/.o/g}
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0x600 ${OBJS}
	${STRIP} ${PROG}.tmp
	objcopy -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>
