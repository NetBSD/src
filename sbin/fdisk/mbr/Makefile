#	$NetBSD: Makefile,v 1.15 2002/11/23 23:45:52 fvdl Exp $

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

.if ${MACHINE} == "i386" || ${MACHINE} == "x86_64"
PROG=	mbr
SRCS=	mbr.S

.if ${MACHINE} == "x86_64"
LDFLAGS+=  -m elf_i386
AFLAGS+=   -m32
.endif

${PROG}: ${OBJS}
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0x600 ${OBJS}
	${STRIP} ${PROG}.tmp
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp
.elif ${MACHINE} == "macppc"
PROG=	mbr
SRCS=	mbr_macppc.S

${PROG}: ${OBJS}
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0x600 ${OBJS}
	${STRIP} ${PROG}.tmp
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp
.endif

.include <bsd.prog.mk>
