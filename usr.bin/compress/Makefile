#	from: @(#)Makefile	5.14 (Berkeley) 6/25/90
#	$Id: Makefile,v 1.3 1993/10/23 01:13:35 jtc Exp $

# USERMEM should be the amount of available user memory in bytes.
# Set it to zero, for physical memory less than 1 Meg.
USERMEM=4194304

PROG=	compress
CFLAGS+=-DBSD4_2 -DSACREDMEM=256000 -DUSERMEM=${USERMEM}

.include <bsd.prog.mk>
