#	$NetBSD: Makefile.inc,v 1.27.74.2 2009/08/19 18:48:19 yamt Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S ffs.S
SRCS+=	memchr.S memcmp.S memcpy.S memmove.S memset.S
SRCS+=	random.S
SRCS+=	strcat.S strchr.S strcmp.S
SRCS+=	strcpy.S strlen.S
SRCS+=	strrchr.S
SRCS+=	scanc.S skpc.S

SRCS+=	crc32.c
