#	$NetBSD: Makefile.walnut.inc,v 1.2 2003/01/04 18:13:51 thorpej Exp $

MKIMG?=	${THISPPC}/compile/walnut-mkimg.sh
CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403
TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	walnut_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/walnut/walnut_start.S

.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
.endif

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} netbsd netbsd.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img
