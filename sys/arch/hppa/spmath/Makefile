#	$NetBSD: Makefile,v 1.3.122.1 2017/12/03 11:36:16 jdolecek Exp $

#	$OpenBSD: Makefile,v 1.4 2000/01/11 21:32:57 mickey Exp $

LIB=	spmath
NOPIC=

SRCS=	dfadd.c dfcmp.c dfdiv.c dfmpy.c dfrem.c dfsqrt.c dfsub.c divsfm.c \
	divsfr.c divsim.c divsir.c divu.S divufr.c divuir.c fcnvff.c fcnvfx.c \
	fcnvfxt.c fcnvxf.c frnd.c impys.S impyu.S mpyaccs.c mpyaccu.c mpys.c \
	mpyscv.c mpyu.c mpyucv.c setovfl.c sfadd.c sfcmp.c sfdiv.c sfmpy.c \
	sfrem.c sfsqrt.c sfsub.c fpudispatch.c

CPPFLAGS+=	${SPMATHCPPFLAGS}

MACHINE=	${XMACHINE}
MACHINE_ARCH=	${XMACHINE_ARCH}

# Files to clean up
CLEANFILES+= ${LIB}.o ${LIB}.po

install:

.include <bsd.lib.mk>

${LIB}.o:: ${OBJS:O}
	@echo building standard ${LIB} library
	@rm -f ${LIB}.o
	@${LD} -r -o ${LIB}.o `${LORDER} ${OBJS} | tsort`

${LIB}.po:: ${POBJS:O}
	@echo building profiled ${LIB} library
	@rm -f ${LIB}.po
	@${LD} -r -o ${LIB}.po `${LORDER} ${POBJS} | tsort`
