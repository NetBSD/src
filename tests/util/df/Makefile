#	$NetBSD: Makefile,v 1.5 2011/08/28 09:39:44 christos Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/util/df

TESTS_SH=	t_df

BINDIR=		${TESTSDIR}
PROG=		h_df
.PATH:  	${NETBSDSRCDIR}/bin/df
SRCS=		df.c getmntinfo.c

LDADD+=		-lutil
DPADD+=		${LIBUTIL}

# Pass -DINTREE to make to test using humanize_number.c in source tree
# directly instead of the one in libc.
.if defined(INTREE)
.PATH:		${NETBSDSRCDIR}/lib/libc/gen
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/include
SRCS+=		humanize_number.c
.endif

.include <bsd.test.mk>
