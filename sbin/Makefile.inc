#	from: @(#)Makefile.inc	5.1 (Berkeley) 5/11/90
#	$Id: Makefile.inc,v 1.7 1993/11/03 06:18:59 cgd Exp $

BINDIR?=	/sbin
CFLAGS+=	-I${.CURDIR}/../../sys
LDFLAGS+=	-static
