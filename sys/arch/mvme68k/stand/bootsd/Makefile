#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.3.28.1 2000/06/26 18:12:37 scw Exp $

RELOC=0x3F0000 

S=	${.CURDIR}/../../../..
DEFS= 
NOMAN=	1
NOLINT=	1
INCPATH=-I${.CURDIR} -I${.CURDIR}/../libsa -I${.CURDIR}/../libbug \
	-I${.CURDIR}/../../include -I${S} -I${S}/lib/libsa
CFLAGS=	-O2 ${INCPATH} ${DEFS} ${COPTS}
CLEANFILES+=bootsd

.include "${S}/arch/mvme68k/stand/bugcrt/Makefile.inc"
.include "${S}/arch/mvme68k/stand/libbug/Makefile.inc"
.include "${S}/arch/mvme68k/stand/libsa/Makefile.inc"

SRCS=   boot.c conf.c version.c
PROG=	bootsd 
LIBS=	${LIBSA} ${LIBBUG}

bootsd: ${OBJS} ${LIBSA} ${BUGCRT} ${LIBBUG}
	${LD} -N -T ${RELOC} -o $@ \
		${SRTOBJ} ${BUGCRT} ${OBJS} ${LIBSA} ${LIBBUG} 

install:
	${INSTALL} -c -m 555 -g bin -o bin ${PROG} ${DESTDIR}${MDEC_DIR}

.include <bsd.prog.mk>
