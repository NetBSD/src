#	from: @(#)Makefile	5.6 (Berkeley) 4/27/91
#	$Id: Makefile,v 1.4 1993/08/01 05:45:23 mycroft Exp $

SUBDIR=	fortune

.ifmake !(install)
SUBDIR+= strfile
.endif

SUBDIR+= datfiles

.include <bsd.subdir.mk>
