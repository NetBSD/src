#	$NetBSD: Makefile,v 1.11 2002/11/23 23:45:52 fvdl Exp $

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

.if ${MACHINE} == "x86_64"
LDFLAGS+=  -m elf_i386
AFLAGS+=   -m32
.endif


${PROG}: ${OBJS}
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0x600 ${OBJS}
	${STRIP} ${PROG}.tmp
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>
