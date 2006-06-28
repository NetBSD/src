#	$NetBSD: Makefile,v 1.19 2006/06/28 15:00:51 lukem Exp $

NOMAN=
BINDIR=	/usr/mdec
BINMODE=444

STRIPFLAG=

.include <bsd.own.mk>

LDFLAGS+= -nostdlib
.if ${OBJECT_FMT} == "ELF"
LDFLAGS+= -Wl,-e,start
.else
LDFLAGS+= -Wl,-e,_start -Wl,-N
.endif

.if ${MACHINE} == "macppc"
PROG=	mbr
SRCS=	mbr_macppc.S

${PROG}: ${OBJS}
	${CC} -o ${PROG}.tmp ${LDFLAGS} -Wl,-Ttext,0x600 ${OBJS}
	${STRIP} ${PROG}.tmp
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp
.endif

.include <bsd.prog.mk>
