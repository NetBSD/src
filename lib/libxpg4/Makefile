.include <bsd.own.mk>

LIB=    xpg4
SHLIB_MAJOR= 1
SHLIB_MINOR= 0
WITH_RUNE= yes
DLRUNE= yes

#XXX
#SRCS=   setlocale.c setrunelocale.c big5.c euc.c mskanji.c utf2.c runetype.c \
#	tolower.c toupper.c
.include "${.CURDIR}/../libc/locale/Makefile.inc" # XXX
SRCS+= strcoll.c strxfrm.c
CPPFLAGS+= -Wall -I${.CURDIR}/../libc/locale
CPPFLAGS+= -I${.CURDIR}/../../include -I${.CURDIR}/../libc/include # XXX
CFLAGS+=-g
.PATH: ${.CURDIR}/../libc/locale ${.CURDIR}/../libc/string

NOMAN= YES

.include <bsd.lib.mk>
