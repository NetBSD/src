#	$NetBSD: Makefile.inc,v 1.5 1998/02/22 08:40:37 mycroft Exp $

KMINCLUDES=	arch/m68k/DEFS.h arch/m68k/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memcpy.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S strncmp.S strncpy.S \
	htonl.S htons.S ntohl.S ntohs.S \
	ashldi3.S ashrdi3.S lshrdi3.S
