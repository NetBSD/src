#	$NetBSD: Makefile,v 1.2 2001/02/04 05:19:14 takemura Exp $

S=	../../../..

GENHDR		=	$S/arch/hpc/include/platid_generated.h
MASK_H		=	$S/arch/hpc/include/platid_mask.h
MASK_C		=	$S/arch/hpc/hpc/platid_mask.c
NAME_C		=	$S/arch/hpc/hpc/platid_name.c
GENERATEDS	=	${GENHDR} ${MASK_H} ${MASK_C} ${NAME_C}

PROG	=	platid_gen
BINDIR	=	/usr/sbin
SRCS	=	platid_gen.c gram.y scan.l

YFLAGS		=
YHEADER		=	1
CPPFLAGS	+=	-I${.CURDIR} -I.
CLEANFILES	+=	gram.h
LEX		=	flex
MAN		=

all: ${PROG} ${GENERATEDS}

DEFFILE		=	$S/arch/hpc/conf/platid.def
PLATIDGEN	=	$S/arch/hpc/hpc/platid_gen/${PROG}

${GENHDR}: ${DEFFILE}
	awk -f ../platid_copyright.awk ${DEFFILE} > ${.TARGET}
	${PLATIDGEN} -header < ${DEFFILE} >> ${GENHDR}
${MASK_H}: ${DEFFILE}
	awk -f ../platid_copyright.awk ${DEFFILE} > ${.TARGET}
	${PLATIDGEN} -mask_h < ${DEFFILE} >> ${MASK_H}
${MASK_C}: ${DEFFILE}
	awk -f ../platid_copyright.awk ${DEFFILE} > ${.TARGET}
	${PLATIDGEN} -mask_c < ${DEFFILE} >> ${MASK_C}
${NAME_C}: ${DEFFILE}
	awk -f ../platid_copyright.awk ${DEFFILE} > ${.TARGET}
	${PLATIDGEN} -name_c < ${DEFFILE} >> ${NAME_C}

.include <bsd.prog.mk>
