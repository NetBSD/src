#	$NetBSD: Makefile,v 1.1 2000/02/06 08:47:11 takemura Exp $

GENHDR=	../../include/platid_generated.h
MASK_H=	../../include/platid_mask.h
MASK_C=	../platid_mask.c
NAME_C=	../platid_name.c
GENERATEDS=	${GENHDR} ${MASK_H} ${MASK_C} ${NAME_C}

PROG=	platid_gen
BINDIR=	/usr/sbin
SRCS=	platid_gen.c gram.y scan.l

YFLAGS=
YHEADER=1
CPPFLAGS+=-I${.CURDIR} -I.
CLEANFILES+=gram.h
LEX=flex
MAN=

all: $(GENERATEDS)

${GENHDR}: ${PROG} ../platid.def
	awk -f ../platid_copyright.awk ../platid.def > ${.TARGET}
	./${PROG} -header < ../platid.def >> ${GENHDR}
${MASK_H}: ${PROG} ../platid.def
	awk -f ../platid_copyright.awk ../platid.def > ${.TARGET}
	./${PROG} -mask_h < ../platid.def >> ${MASK_H}
${MASK_C}: ${PROG} ../platid.def
	awk -f ../platid_copyright.awk ../platid.def > ${.TARGET}
	./${PROG} -mask_c < ../platid.def >> ${MASK_C}
${NAME_C}: ${PROG} ../platid.def
	awk -f ../platid_copyright.awk ../platid.def > ${.TARGET}
	./${PROG} -name_c < ../platid.def >> ${NAME_C}

.include <bsd.prog.mk>
