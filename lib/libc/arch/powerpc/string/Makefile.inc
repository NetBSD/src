#	$NetBSD: Makefile.inc,v 1.12 2009/08/11 17:30:43 dsl Exp $

SRCS+=  bzero.S ffs.S strlen.S
NO_SRCS+= memset.S

# disable the asm versions of these because they break the explora.
# the special rules here are to override the suffix rules which seem
# to prefer .S files over .c
.if ${MACHINE} == "evbppc"
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
SRCS+=	memcmp.S bcopy.S memcpy.S memmove.S
.endif
