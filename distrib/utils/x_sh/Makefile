# 	$NetBSD: Makefile,v 1.5 1998/05/03 16:50:25 ross Exp $
# Build a smaller sh (e.g. for boot media)
# 
# Once upon a time (actually, 11/97) cjs wrote this Makefile and put
# in a comment explaining why the obvious approach would NOT work,
# and proceeded to do it in an ugly but functional way. Later (actually,
# on 3/29/98) Christos came along and did it the simple, clean, and
# beautiful way which, sadly, does not work. What's funny is, that pesky
# comment was deleted in the same 3/29 commit.
# 
# So now, on 5/3/98 ross comes along and decides to fix it. I'm not doing
# it Curt's way, exactly, but I am going to explain again why this is ugly
# and, once again, why the obvious way to do it won't work. The reason for
# this long-winded explaination is obvious: if someone comes along and "fixes"
# my ugly version I want them to at least get their version to really work.
# 
# You cannot completely test this makefile by building in the
# utils/x_sh directory!  The whole problem comes from crunchgen(1).
# The way crunchgen(1) works is to build a makefile in /tmp that includes
# the makefile from the directory to be crunched. It executes that makefile
# in order to get the list of files to work on. The key thing is: crunchgen
# doesn't run from this directory. Therefore, this makefile has to be able
# to work from some other directory.
#
# So, my XXX hack is to locate the sh directory from the different
# possible subdirectory level counts beneath /usr/src. Note that using
# objdirs changes the level count and usually makes it impossible to do a
# relative traversal to another source directory. That's why the :H is
# used, to cancel out the objdir. ".." won't do it, that moves us up
# the obj tree.
#
# We could just use only the BSDSRCDIR assignment, but right now the use
# of BSDSRCDIR can be deleted here, and so I didn't want to depend on it
# in case someday we completely nuke BSDSRCDIR.

NOMAN=1

.include <bsd.own.mk>

.for i in     ../../bin/sh		\
	   ../../../bin/sh		\
	../../../../bin/sh
.if exists(${.CURDIR:H}/$i/Makefile)
SRCDIR?= ${.CURDIR:H}/$i
.endif
.endfor

SRCDIR?= ${BSDSRCDIR}/bin/sh

CPPFLAGS+= -I${SRCDIR} -DSMALL

.PATH: ${SRCDIR} ${SRCDIR}/bltin ${SRCDIR}/../../usr.bin/printf

.include "${SRCDIR}/Makefile"
