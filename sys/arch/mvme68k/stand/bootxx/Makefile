#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.4 2000/06/25 16:39:06 scw Exp $

RELOC=0x3F0000 

S=	${.CURDIR}/../../../..
DEFS= 
NOMAN=	1
NOLINT=	1
INCPATH=-I${.CURDIR} -I${.CURDIR}/../libsa -I${.CURDIR}/../libbug \
	-I${.CURDIR}/../../include -I${S} -I${S}/lib/libsa
CFLAGS=	-O2 ${INCPATH} ${DEFS} ${COPTS}
CLEANFILES+=bootxx

.include "${S}/arch/mvme68k/stand/bugcrt/Makefile.inc"
.include "${S}/arch/mvme68k/stand/libbug/Makefile.inc"
.include "${S}/arch/mvme68k/stand/libsa/Makefile.inc"

SRCS=   bootxx.c conf.c version.c
PROG=	bootxx
LIBS=	${LIBSA} ${LIBBUG}

${PROG}: ${OBJS} ${BUGCRT} ${LIBS}
	${LD} -N -T ${RELOC} ${BUGCRT} ${OBJS} ${LIBS} -o $@

install:
	${INSTALL} -c -m 555 -g bin -o bin ${PROG} ${DESTDIR}${MDEC_DIR}

.include <bsd.prog.mk>
