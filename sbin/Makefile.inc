#	from: @(#)Makefile.inc	5.1 (Berkeley) 5/11/90
#	$Id: Makefile.inc,v 1.6 1993/10/01 01:53:46 mycroft Exp $

BINDIR?=/sbin
CFLAGS+=-I${.CURDIR}/../../sys
