#	$Id: Makefile,v 1.3 1993/10/06 21:28:25 cgd Exp $

# Define FSYNC_ALL to get slower but safer writes in case of crashes in
# the middle of CVS/RCS changes
CFLAGS += -DFSYNC_ALL

LIB     = rcs
SRCS    = maketime.c partime.c rcsedit.c rcsfcmp.c rcsfnms.c rcsgen.c \
	  rcskeep.c rcskeys.c rcslex.c rcsmap.c rcsrev.c rcssyn.c rcsutil.c \
	  merger.c

NOPROFILE=noprofile
NOPIC=nopic

install:

.include <bsd.lib.mk>
