#	$NetBSD: Makefile.ev64260.inc,v 1.2.2.1 2005/04/01 14:27:26 skrll Exp $

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh

TEXTADDR?=	300000

SYSTEM_FIRST_OBJ=	${BOARDTYPE}_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/${BOARDTYPE}/${BOARDTYPE}_locore.S

.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
.endif

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} netbsd netbsd.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img
