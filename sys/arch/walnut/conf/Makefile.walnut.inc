#	$NetBSD: Makefile.walnut.inc,v 1.1.8.3 2002/04/01 07:43:34 nathanw Exp $

MKIMG?=	${THISPPC}/compile/mkimg.sh
CFLAGS+=-mcpu=403
TEXTADDR?=	25000
.ifdef DBSYM
SYSTEM_LD_TAIL_EXTRA+=; ${DBSYM} $@
.endif

netbsd.img: netbsd
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; ${MKIMG} netbsd netbsd.img

