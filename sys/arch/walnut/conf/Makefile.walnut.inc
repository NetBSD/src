#	$NetBSD: Makefile.walnut.inc,v 1.3.6.1 2002/07/15 01:41:04 gehenna Exp $

MKIMG?=	${THISPPC}/compile/mkimg.sh
CFLAGS+=-mcpu=403
TEXTADDR?=	25000
.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; echo ${DBSYM} $@; ${DBSYM} $@
.endif
SYSTEM_LD_TAIL_EXTRA+=; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img

