#	$NetBSD: Makefile,v 1.4 2011/05/22 12:01:26 plunky Exp $

.include <bsd.init.mk>

.PATH:	${PCC_LIBS}/libpcc

LIB=	pcc

SRCS=	cmpdi2.c divdi3.c fixdfdi.c fixsfdi.c fixunsdfdi.c \
	fixunssfdi.c floatdidf.c floatdisf.c floatunsdidf.c \
	moddi3.c muldi3.c negdi2.c qdivrem.c \
	ucmpdi2.c udivdi3.c umoddi3.c 
SRCS+=	ashldi3.c ashrdi3.c lshrdi3.c
SRCS+=	ssp.c

COPTS.ssp.c+=	-Wno-error

.if ${TARGET_ENDIANNESS} == "1234"
CPPFLAGS+=      -D__LITTLE_ENDIAN__
.elif ${TARGET_ENDIANNESS} == "4321"
CPPFLAGS+=      -D__BIG_ENDIAN__
.else
ERROR!= echo "ERROR: Unknown TARGET_ENDIANNESS" >&2;echo
.endif

.include <bsd.lib.mk>
