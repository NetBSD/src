#	from: @(#)Makefile	5.14 (Berkeley) 6/25/90
#	$Id: Makefile,v 1.2 1993/07/31 15:24:21 mycroft Exp $

# USERMEM should be the amount of available user memory in bytes.
# Set it to zero, for physical memory less than 1 Meg.
USERMEM=4194304

PROG=	compress
CFLAGS+=-DBSD4_2 -DSACREDMEM=256000 -DUSERMEM=${USERMEM}
LINKS=	${BINDIR}/compress ${BINDIR}/uncompress \
	${BINDIR}/compress ${BINDIR}/zcat
MLINKS=	compress.1 uncompress.1 compress.1 zcat.1

.include <bsd.prog.mk>
