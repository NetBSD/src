#       $NetBSD: Makefile.inc,v 1.3 2009/07/17 19:37:55 dsl Exp $

SRCS+=  bcmp.S bcopy.S bzero.S ffs.S memchr.c memcmp.S memset.S \
        strcat.c strcmp.c strcpy.c strcspn.c strlen.c \
        strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
        strspn.c strstr.c swab.c
SRCS+=	memcpy.S memmove.S
