#	$Id: Makefile.inc,v 1.2 1993/08/02 17:48:11 mycroft Exp $

.if exists(${.CURDIR}/../lib/obj)
LIBRCS=	${.CURDIR}/../lib/obj/librcs.a
.else
LIBRCS=	${.CURDIR}/../lib/librcs.a
.endif
