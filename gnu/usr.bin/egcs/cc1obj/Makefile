#	$NetBSD: Makefile,v 1.4 1998/04/02 06:39:52 mrg Exp $

PROG=	cc1obj
SRCS=	objc-parse.y objc-act.c 

# XXX needs to be before dist/gcc or tree.c comes from the
# wrong place..
.PATH: $(.CURDIR)/../../../dist/gcc/objc

.include "../Makefile.cc1"

CPPFLAGS+=	-I$(DIST)/objc
