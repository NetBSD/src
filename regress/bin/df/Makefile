#	$NetBSD: Makefile,v 1.4 2007/03/16 01:29:40 enami Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

PROG=	df
.PATH:  ${NETBSDSRCDIR}/bin/df
.PATH:  ${NETBSDSRCDIR}/bin/csh
SRCS=	df.c strpct.c getmntinfo.c

# Pass -DINTREE to make to test using humanize_number.c in source tree
# directly instead of the one in libc.
.if defined(INTREE)
.PATH:		${NETBSDSRCDIR}/lib/libc/gen
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/include
SRCS+=		humanize_number.c
.endif

regress:
	env BLOCKSIZE=1k ./df | diff -w ${.CURDIR}/df.out -
	env BLOCKSIZE=1k ./df -h | diff -w ${.CURDIR}/df-h.out -

.include <bsd.prog.mk>
