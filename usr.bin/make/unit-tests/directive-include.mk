# $NetBSD: directive-include.mk,v 1.2 2020/10/31 22:55:35 rillig Exp $
#
# Tests for the .include directive, which includes another file.

# TODO: Implementation

.MAKEFLAGS: -dc

# All included files are recorded in the variable .MAKE.MAKEFILES.
# In this test, only the basenames of the files are compared since
# the directories can differ.
.include "/dev/null"
.if ${.MAKE.MAKEFILES:T} != "${.PARSEFILE} null"
.  error
.endif

# Each file is recorded only once in the variable .MAKE.MAKEFILES.
# XXX: As of 2020-10-31, the very last file can be repeated, due to an
# off-by-one bug in ParseTrackInput.
.include "/dev/null"
.if ${.MAKE.MAKEFILES:T} != "${.PARSEFILE} null null"
.  error
.endif

# Since the file /dev/null is not only recorded at the very end of the
# variable .MAKE.MAKEFILES, it is not added a third time.
.include "/dev/null"
.if ${.MAKE.MAKEFILES:T} != "${.PARSEFILE} null null"
.  error
.endif

all:
	@:;
