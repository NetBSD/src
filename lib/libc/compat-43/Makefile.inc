#	from: @(#)Makefile.inc	5.3 (Berkeley) 2/20/91
#	$Id: Makefile.inc,v 1.2 1993/08/01 05:41:29 mycroft Exp $

# compat-43 sources
.PATH: ${.CURDIR}/${MACHINE}/compat-43 ${.CURDIR}/compat-43

SRCS+=	creat.c getwd.c killpg.c setpgrp.c sigcompat.c

MAN2+=	creat.0 killpg.0 sigblock.0 sigpause.0 sigsetmask.0 sigvec.0
