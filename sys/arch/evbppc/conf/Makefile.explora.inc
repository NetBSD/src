#	$NetBSD: Makefile.explora.inc,v 1.2 2003/05/31 12:00:59 hannken Exp $

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403

SYSTEM_FIRST_OBJ=	explora_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/explora/explora_start.S

.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true
.endif

SYSTEM_LD_TAIL_EXTRA+=; \
	echo objcopy --adjust-vma 0x81000000 netbsd netbsd.img ; \
	${OBJCOPY} --adjust-vma 0x81000000 netbsd netbsd.img
