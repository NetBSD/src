#	$NetBSD: Makefile,v 1.20 2006/06/28 15:03:00 lukem Exp $

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

CLEANFILES+=	${PROG}.tmp

${PROG}: ${OBJS}
	${_MKTARGET_LINK}
	${CC} -o ${PROG}.tmp ${LDFLAGS} -Wl,-Ttext,0x600 ${OBJS}
	${STRIP} ${PROG}.tmp
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}

.endif	# macppc

.include <bsd.prog.mk>
