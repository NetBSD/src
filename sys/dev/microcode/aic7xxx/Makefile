#	$NetBSD: Makefile,v 1.7 2003/04/19 19:26:10 fvdl Exp $

7XXX_SEQ=	aic7xxx.seq
7XXX_SEQHDR=	aic7xxx_seq.h
7XXX_REGHDR=	aic7xxx_reg.h

79XX_SEQ=	aic79xx.seq
79XX_SEQHDR=	aic79xx_seq.h
79XX_REGHDR=	aic79xx_reg.h

AICASMINCS=-I/usr/include -I${.CURDIR}/../../.. -I.

PROG=	aicasm

SRCS=	aicasm.c aicasm_symbol.c aicasm_scan.l aicasm_macro_scan.l \
	aicasm_gram.y aicasm_macro_gram.y

YHEADER=1

DPADD+= ${LIBL} ${LIBY}
LDADD+= -ll -ly

NOSHARE=# defined

.include <bsd.prog.mk>

#
# XXXfvdl must come after the .include. Yes, it's ugly.
#
LFLAGS+= ${.TARGET:M*macro*:S/$(.TARGET)/-Pmm/}
YFLAGS+= ${.TARGET:M*macro*:S/$(.TARGET)/-p mm/}

regen: 7xxx_headers 79xx_headers

7xxx_headers: ${PROG} ${7XXX_SEQ}
	./${PROG} ${AICASMINCS} -o ${7XXX_SEQHDR} -r ${7XXX_REGHDR} ${7XXX_SEQ}

79xx_headers: ${PROG} ${79XX_SEQ}
	./${PROG} ${AICASMINCS} -o ${79XX_SEQHDR} -r ${79XX_REGHDR} ${79XX_SEQ}
