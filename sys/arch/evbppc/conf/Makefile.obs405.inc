#	$NetBSD: Makefile.obs405.inc,v 1.1.2.1 2005/03/22 19:07:44 tron Exp $

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh

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
