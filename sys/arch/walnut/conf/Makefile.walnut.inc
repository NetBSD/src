#	$NetBSD: Makefile.walnut.inc,v 1.1.4.5 2002/06/23 17:43:11 jdolecek Exp $

MKIMG?=	${THISPPC}/compile/mkimg.sh
CFLAGS+=-mcpu=403
TEXTADDR?=	25000
.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; echo ${DBSYM} $@; ${DBSYM} $@
.endif
SYSTEM_LD_TAIL_EXTRA+=; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img

