#	$NetBSD: Makefile,v 1.13 2001/09/22 02:01:13 tv Exp $

MAN=	mbr.8
MLINKS=	mbr.8 bootselect.8

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
	${STRIP} ${PROG}.tmp
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp
.endif

.include <bsd.prog.mk>
