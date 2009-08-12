#	$NetBSD: Makefile.inc,v 1.17 2009/08/12 21:20:41 dsl Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
SRCS+=	scanc.c skpc.c
SRCS+=	strcat.c strcmp.c strcpy.c strlen.c strcasecmp.c
SRCS+=	strncasecmp.c strncmp.c strtoul.c
SRCS+=	strncpy.c random.c __assert.c memchr.c memcmp.c

SRCS+=	ffs.S
SRCS+=	memset.S
SRCS+=	memmove.S memcpy.S byte_swap_2.S byte_swap_4.S byte_swap_8.S
SRCS+=	ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S movstr_i4.S
SRCS+=	movstrSI.S movstrSI12_i4.S mulsi3.S sdivsi3.S udivsi3.S
