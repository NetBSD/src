#	$NetBSD: Makefile,v 1.2.198.1 2017/12/03 11:35:49 jdolecek Exp $
#
TARGET	= loadbsd

CC	= gcc
CFLAGS	= -D_STANDALONE -I./include -O -fomit-frame-pointer -msmall-code
LDFLAGS	= -noixemul
LDLIBS	=

OBJS	= loadbsd.o loadfile.o loadfile_aout.o loadfile_elf32.o getopt.o

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
