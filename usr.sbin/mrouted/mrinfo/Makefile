#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp
#	$Id: Makefile,v 1.3 1995/06/01 02:26:16 mycroft Exp $

.PATH: ${.CURDIR}/..
CFLAGS+= -I${.CURDIR}/..

PROG=	mrinfo
SRCS=	igmp.c inet.c kern.c mrinfo.c
NOMAN=	noman
BINOWN= root
BINMODE=4555


.include "../../Makefile.inc"
.include <bsd.prog.mk>
