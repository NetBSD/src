#	$NetBSD: Makefile.obs405.inc,v 1.1.4.2 2004/08/03 10:34:16 skrll Exp $

MKIMG?=	${THISPPC}/compile/walnut-mkimg.sh

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403
TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	locore.o
SYSTEM_FIRST_SFILE=	${POWERPC}/${PPCDIR}/openbios/locore.S

.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
.endif

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} netbsd netbsd.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img
