#	$NetBSD: Makefile,v 1.3 2004/07/27 01:50:35 enami Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

PROG=	df
.PATH:  ${NETBSDSRCDIR}/bin/df
.PATH:  ${NETBSDSRCDIR}/bin/csh
SRCS=	df.c strpct.c getmntinfo.c

# Pass -DINTREE to make to test using humanize_number.c in source tree
# directly instead of the one in libc.
.if defined(INTREE)
.PATH:  ${NETBSDSRCDIR}/lib/libc/gen
SRCS+=	humanize_number.c
.endif

regress:
	env BLOCKSIZE=1k ./df | cmp ${.CURDIR}/df.out -
	env BLOCKSIZE=1k ./df -h | cmp ${.CURDIR}/df-h.out -

.include <bsd.prog.mk>
