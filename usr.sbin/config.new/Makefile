#	@(#)Makefile	5.1 (Berkeley) 1/12/93

YACC= byacc
LEX = flex
CC= 	gcc
PROG=	config.new
SRCS=	files.c gram.y hash.c main.c mkheaders.c mkioconf.c mkmakefile.c \
	mkswap.c pack.c scan.l sem.c util.c
CFLAGS=-I${.CURDIR} -I. -g
CLEANFILES=gram.c scan.c y.tab.h
MAN8=	config.0

.include <bsd.prog.mk>

.depend: gram.c scan.c
