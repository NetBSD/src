#	$NetBSD: Makefile,v 1.1.8.2 2008/06/02 13:23:34 mjf Exp $

BIN=	bin2blob array2bin

all:	${BIN}

bin2blob:	bin2blob.c	
		cc -Wall -O2 -lz -o bin2blob bin2blob.c

array2bin:	array2bin.c	
		cc -Wall -O2 -lz -o array2bin array2bin.c

clean:
		rm -f ${BIN} *~
