#	$NetBSD: Makefile.walnut.inc,v 1.6 2002/11/25 02:56:20 thorpej Exp $

MKIMG?=	${THISPPC}/compile/mkimg.sh
CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403
TEXTADDR?=	25000
.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; echo ${DBSYM} $@; ${DBSYM} $@
.endif

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} netbsd netbsd.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img
