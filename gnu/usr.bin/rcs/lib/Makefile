#	$NetBSD: Makefile,v 1.12.2.1 1997/11/08 06:54:17 lukem Exp $

# Define FSYNC_ALL to get slower but safer writes in case of crashes in
# the middle of CVS/RCS changes
CPPFLAGS+=-DFSYNC_ALL

# If you wish to use a different Id token in addition to Id,
# uncomment the following line and edit to taste.
#CPPFLAGS+=-DLOCALID=\"Foo\"

LIB=	rcs
SRCS=	maketime.c merger.c partime.c rcsedit.c rcsfcmp.c rcsfnms.c rcsgen.c \
	rcskeep.c rcskeys.c rcslex.c rcsmap.c rcsrev.c rcssyn.c rcstime.c \
	rcsutil.c version.c

NOPROFILE=noprofile
NOPIC=nopic

# only needed during build
libinstall::

.include <bsd.lib.mk>
