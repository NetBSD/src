#	$NetBSD: Makefile,v 1.3.36.1 2009/05/13 19:19:44 jym Exp $

PROG=	banner

SRCS=	$(PROG).c chset.c

# if you want to make chars from the letter involved (like lpd), then
# un-comment the following.
#CPPFLAGS+= -DNOHASH_ONLY

# if you want to use the same char set that lpd uses then un-comment
# the following.  Lpd uses a much nicer character set but banner
# output will not be the same as the real thing.
#CPPFLAGS+= -DLPD_CHSET

.include <bsd.prog.mk>

