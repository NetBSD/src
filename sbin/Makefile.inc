#	from: @(#)Makefile.inc	5.1 (Berkeley) 5/11/90
#	$Id: Makefile.inc,v 1.8 1993/11/08 02:33:56 cgd Exp $

BINDIR?=	/sbin
CFLAGS+=	-I${.CURDIR}/../../sys
LDSTATIC=	-static
