#	$NetBSD: Makefile.inc,v 1.7 1997/10/09 10:07:46 lukem Exp $

KMINCLUDES= arch/ns32k/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S memcpy.S memset.S strcat.S strcmp.S \
	strcpy.S strlen.S strncmp.S strncpy.S htonl.S htons.S ntohl.S ntohs.S
