#	$NetBSD: Makefile.inc,v 1.17 2003/08/13 11:32:23 ragge Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__assert.c __main.c blkcpy.S blkset.S bswap64.S random.S \
	strcasecmp.c strncasecmp.c strncmp.c strtoul.c udiv.S urem.S \
	strncpy.c memcmp.c strcat.c strcpy.c strcmp.c strlen.c memchr.c
