#	$NetBSD: Makefile.inc,v 1.14 2021/07/24 05:27:25 rin Exp $

SRCS+=  bzero.S ffs.S strlen.S
NO_SRCS+= memset.S

# disable the asm versions of these because they break the explora.
# the special rules here are to override the suffix rules which seem
# to prefer .S files over .c
.if ${MACHINE} == "evbppc"
.  for name in bcopy memcmp memcpy memmove
.    for suffix in o po pico go
${name}.${suffix}: ${name}.c
.    endfor
.  endfor
.else
SRCS+=	memcmp.S bcopy.S memcpy.S memmove.S
.endif
