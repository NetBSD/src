#	$NetBSD: Makefile.inc,v 1.10 1996/11/30 02:06:27 jtc Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S strncmp.S \
	strncpy.S htonl.S htons.S ntohl.S ntohs.S scanc.S skpc.S \
	strncasecmp.c __assert.c

SRCS+= ashldi3.S ashrdi3.S lshrdi3.S
