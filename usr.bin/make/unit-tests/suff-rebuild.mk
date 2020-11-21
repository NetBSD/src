# $NetBSD: suff-rebuild.mk,v 1.4 2020/11/21 08:51:57 rillig Exp $
#
# Demonstrates what happens to transformation rules (called inference rules
# by POSIX) when all suffixes are deleted.

all: suff-rebuild-example

.SUFFIXES:

.SUFFIXES: .a .b .c

suff-rebuild-example.a:
	: Making ${.TARGET} out of nothing.

.a.b:
	: Making ${.TARGET} from ${.IMPSRC}.
.b.c:
	: Making ${.TARGET} from ${.IMPSRC}.
.c:
	: Making ${.TARGET} from ${.IMPSRC}.

# XXX: At a quick glance, the code in SuffUpdateTarget looks as if it were
# possible to delete the suffixes in the middle of the makefile, add back
# the suffixes from before, and have the transformation rules preserved.
#
# As of 2020-09-25, uncommenting the following line results in the error
# message "don't know how to make suff-rebuild-example" though.
#
#.SUFFIXES:

# Add the suffixes back.  It should not matter that the order of the suffixes
# is different from before.
.SUFFIXES: .c .b .a
