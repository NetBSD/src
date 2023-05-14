#	$NetBSD: Makefile,v 1.5 2023/05/14 16:13:05 phx Exp $
# Set sdkprefix to your gcc-amiga toolchain as needed.
TARGET	= loadbsd

sdkprefix =
CC	= $(sdkprefix)gcc
CFLAGS	= -D_STANDALONE -DNO_MID_CHECK -I./include -Os -fomit-frame-pointer -msmall-code -m68020
AS	= $(sdkprefix)as
ASFLAGS	=
#LDFLAGS	= -noixemul
LDFLAGS	= -mcrt=nix20
LDLIBS	=

OBJS	= loadbsd.o startit.o loadfile.o loadfile_aout.o loadfile_elf32.o

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
