#	$NetBSD: Makefile.walnut.inc,v 1.4 2002/05/30 15:24:57 eeh Exp $

MKIMG?=	${THISPPC}/compile/mkimg.sh
CFLAGS+=-mcpu=403
TEXTADDR?=	25000
.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; echo ${DBSYM} $@; ${DBSYM} $@
.endif
SYSTEM_LD_TAIL_EXTRA+=; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img

