#	$NetBSD: Makefile.inc,v 1.31.70.1 2020/02/29 20:21:03 ad Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S ffs.S
SRCS+=	memchr.S memcpy.S memmove.S memset.S
SRCS+=	random.S
SRCS+=	strcat.S strchr.S strcmp.S
SRCS+=	strcpy.S strlen.S
SRCS+=	strrchr.S
SRCS+=	scanc.S skpc.S

SRCS+=	crc32.c
