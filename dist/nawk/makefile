# /****************************************************************
# Copyright (C) Lucent Technologies 1997
# All Rights Reserved
# 
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby
# granted, provided that the above copyright notice appear in all
# copies and that both that the copyright notice and this
# permission notice and warranty disclaimer appear in supporting
# documentation, and that the name Lucent Technologies or any of
# its entities not be used in advertising or publicity pertaining
# to distribution of the software without specific, written prior
# permission.
# 
# LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
# IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
# SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
# IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
# ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
# THIS SOFTWARE.
# ****************************************************************/

CFLAGS = -g
CFLAGS = -O2
CFLAGS =

CC = gcc -Wall -g
CC = /opt/pure/purify/purify cc
CC = cc

YACC = bison -y
YACC = yacc
YFLAGS = -d

OFILES = b.o main.o parse.o proctab.o tran.o lib.o run.o lex.o

SOURCE = awk.h awkgram.c awkgram.h proto.h awkgram.y lex.c b.c main.c maketab.c parse.c lib.c run.c tran.c proctab.c missing95.c

LISTING = awk.h proto.h awkgram.y lex.c b.c main.c maketab.c parse.c lib.c run.c tran.c missing95.c

SHIP = README FIXES $(SOURCE) awkgram[ch].bak makefile awk.1 buildwin.bat mac.code

a.out:	awkgram.o $(OFILES)
	$(CC) $(CFLAGS) awkgram.o $(OFILES) $(ALLOC)  -lm

$(OFILES):	awk.h awkgram.h proto.h

awkgram.o:	awk.h proto.h awkgram.y
	$(YACC) $(YFLAGS) awkgram.y
	mv y.tab.c awkgram.c
	mv y.tab.h awkgram.h
	$(CC) $(CFLAGS) -c awkgram.c

proctab.c:	maketab
	./maketab >proctab.c

maketab:	awkgram.h maketab.c
	$(CC) $(CFLAGS) maketab.c -o maketab

bundle:
	@cp awkgram.h awkgramh.bak
	@cp awkgram.c awkgramc.bak
	@bundle $(SHIP)

tar:
	@cp awkgram.h awkgramh.bak
	@cp awkgram.c awkgramc.bak
	@bundle $(SHIP) >awk.shar
	@tar cf awk.tar $(SHIP)
	gzip awk.tar
	ls -l awk.tar.gz
	@zip awk.zip $(SHIP)
	ls -l awk.zip

names:
	@echo $(LISTING)

clean:
	rm -f a.out *.o maketab # proctab.c
