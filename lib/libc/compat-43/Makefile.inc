#	from: @(#)Makefile.inc	5.3 (Berkeley) 2/20/91
#	$Id: Makefile.inc,v 1.10.2.1 1995/04/25 23:39:04 jtc Exp $

# compat-43 sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/compat-43 ${.CURDIR}/compat-43

SRCS+=	creat.c getdtablesize.c gethostid.c \
	getwd.c killpg.c sethostid.c setpgrp.c sigcompat.c

MAN+=	creat.3 getdtablesize.3 gethostid.3 killpg.3 \
	sigblock.3 sigpause.3 sigsetmask.3 sigvec.3

MLINKS+=gethostid.3 sethostid.3
