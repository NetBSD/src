#	$NetBSD: Makefile,v 1.1 2001/07/26 22:11:08 bjh21 Exp $

S?=	${.CURDIR}/../../../../

LIB= riscos
MKPIC=no
MKPROFILE=no

CPPFLAGS= -I$S/lib/libsa ${RISCOSCPPFLAGS} ${RISCOSMISCCPPFLAGS}
#CPPFLAGS+= -DDISK_DEBUG
#CPPFLAGS+= -DNO_DISKLABEL
#CPPFLAGS+= -DSAVE_MEMORY

SRCS=	srt1.c devopen.c
SRCS+=	riscoscalls.S riscoscons.c riscoserrors.c riscosfile.c

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`
