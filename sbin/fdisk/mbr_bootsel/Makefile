#	$NetBSD: Makefile,v 1.2 2000/02/01 06:01:21 tsutsui Exp $

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
	${STRIP} ${PROG}.tmp
	objcopy -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>
