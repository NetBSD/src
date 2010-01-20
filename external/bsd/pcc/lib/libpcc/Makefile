#	$NetBSD: Makefile,v 1.3 2010/01/20 11:45:55 plunky Exp $

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

.include <bsd.lib.mk>
