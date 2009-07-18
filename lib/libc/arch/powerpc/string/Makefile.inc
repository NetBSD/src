#	$NetBSD: Makefile.inc,v 1.9 2009/07/18 09:44:30 dsl Exp $

# disable the asm versions of these because they break the explora.
# the special rules here are to override the suffix rules which seem
# to prefer .S files over .c
.if ${MACHINE} == "evbppc"
SRCS+=  bzero.S ffs.S strlen.S memcmp.c bcopy.c
bcopy.o: bcopy.c
bcopy.po: bcopy.c
bcopy.so: bcopy.c
memcpy.o: memcpy.c
memcpy.po: memcpy.c
memcpy.so: memcpy.c
memcmp.o: memcmp.c
memcmp.po: memcmp.c
memcmp.so: memcmp.c
memmove.o: memmove.c
memmove.po: memmove.c
memmove.so: memmove.c
.else
SRCS+=	bzero.S ffs.S strlen.S memcmp.S bcopy.S memcpy.S memmove.S
.endif

SRCS+=	bcmp.c memchr.c \
	strcat.c strcmp.c strcpy.c strcspn.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
