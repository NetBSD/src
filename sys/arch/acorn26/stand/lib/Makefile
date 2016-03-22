#	$NetBSD: Makefile,v 1.4 2016/03/22 08:25:22 mrg Exp $

S?=	${.CURDIR}/../../../../

LIB= riscos
NOPIC=# defined
NOPROFILE=# defined

CPPFLAGS= -I$S/lib/libsa ${RISCOSCPPFLAGS} ${RISCOSMISCCPPFLAGS}
#CPPFLAGS+= -DDISK_DEBUG
#CPPFLAGS+= -DNO_DISKLABEL
#CPPFLAGS+= -DSAVE_MEMORY

SRCS=	srt1.c devopen.c
SRCS+=	riscoscalls.S riscoscons.c riscoserrors.c riscosfile.c

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS:O}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`
