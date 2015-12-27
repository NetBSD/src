#	$NetBSD: Makefile,v 1.2.218.1 2015/12/27 12:09:29 skrll Exp $
#
TARGET	= loadbsd

CC	= gcc
CFLAGS	= -D_STANDALONE -I./include -O -fomit-frame-pointer -msmall-code
LDFLAGS	= -noixemul
LDLIBS	=

OBJS	= loadbsd.o loadfile.o loadfile_aout.o loadfile_elf32.o getopt.o

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
