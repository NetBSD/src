#	$NetBSD: Makefile.walnut.inc,v 1.3.8.2 2003/08/12 18:28:35 tron Exp $

MKIMG?=	${THISPPC}/compile/mkimg.sh
CFLAGS+=-mcpu=403
TEXTADDR?=	25000
.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; echo ${DBSYM} $@; ${DBSYM} $@
.endif
SYSTEM_LD_TAIL_EXTRA+=; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		sh ${MKIMG} $@ $@.img

