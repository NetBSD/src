#	$NetBSD: Makefile.boot,v 1.5 2002/09/22 06:22:50 dbj Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94
#
# a very simple makefile...
#
# You only want to use this if you aren't running NetBSD.
#
CC=gcc -O
CFLAGS= -I. -DMAKE_BOOTSTRAP

# Uncomment this if your system does not have strtoul (i.e. SunOS)
# STRTOUL= -Dstrtoul=strtol

# Note: The scanner here uses features specific to "flex" so
# do not bother even trying to make lex build the scanner.
# If you do not have flex, the source can be found in:
# src/usr.bin/lex (See Makefile.boot)
LEX=flex -l

YACC=yacc

OBJS=	files.o hash.o main.o mkdevsw.o mkheaders.o mkioconf.o \
	mkmakefile.o mkswap.o pack.o sem.o util.o \
	gram.o lex.yy.o strerror.o

config: ${OBJS}
	${CC} -o $@ ${OBJS}

gram.o : gram.c
	${CC} ${CFLAGS} -c gram.c

gram.c gram.h : gram.y
	${YACC} -d gram.y
	-mv -f y.tab.c gram.c
	-mv -f y.tab.h gram.h

lex.yy.o : lex.yy.c gram.h
	${CC} ${CFLAGS} ${STRTOUL} -c lex.yy.c

lex.yy.c : scan.l
	${LEX} scan.l

${OBJS} : defs.h

y.tab.o mkmakefile.o mkswap.o sem.o : sem.h
lex.yy.o : gram.h

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	rm -f *.o config lex.yy.c y.tab.[ch] gram.[ch]

