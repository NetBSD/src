#	$Id: Makefile.inc,v 1.4 1997/04/25 01:04:15 thorpej Exp $

KMINCLUDES= arch/i386/SYS.h
KMSRCS=	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S \
	htonl.S htons.S ntohl.S ntohs.S
