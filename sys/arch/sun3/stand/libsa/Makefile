#	$NetBSD: Makefile,v 1.2 1995/02/24 05:07:01 gwr Exp $

LIB=sa

SRCS=	 alloc.c ashrdi3.c bcopy.c bzero.c gets.c open.c \
	 panic.c printf.c promcons.c promdev.c read.c \
	 strcmp.c strerror.c strlen.c strlen.c ufs.c 

NOPIC=nopic
NOPROFILE=noprofile

# Logically src/sys
S=../../../..

SRC_SA=$S/lib/libsa
SRC_KERN=$S/lib/libkern

DEFS= -DCOMPAT_UFS
INCL= -I. -I${S}/lib/libsa -I${S}
CFLAGS= -O -msoft-float ${DEFS} ${INCL}

.PATH:  ${SRC_SA} ${SRC_KERN}

all: libsa.a SRT0.o SRT1.o

.include <bsd.lib.mk>
