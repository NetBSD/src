#	$NetBSD: Makefile,v 1.22 2008/05/03 14:48:30 lukem Exp $

LIBISPRIVATE=	yes

# Define FSYNC_ALL to get slower but safer writes in case of crashes in
# the middle of CVS/RCS changes
CPPFLAGS+=-DFSYNC_ALL

.include <bsd.own.mk>		# for RCS_LOCALID

# If you wish to use a different Id token in addition to Id,
# set the variable RCS_LOCALID in mk.conf or environment.
.if defined(RCS_LOCALID)
CPPFLAGS+=-DLOCALID=\"${RCS_LOCALID}\"
.endif

LIB=	rcs
SRCS=	maketime.c merger.c partime.c rcsedit.c rcsfcmp.c rcsfnms.c rcsgen.c \
	rcskeep.c rcskeys.c rcslex.c rcsmap.c rcsrev.c rcssyn.c rcstime.c \
	rcsutil.c version.c

.include <bsd.lib.mk>
