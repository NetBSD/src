# $NetBSD: directive-elif.mk,v 1.3 2020/11/03 17:17:31 rillig Exp $
#
# Tests for the .elif directive.

# TODO: Implementation

.info begin .elif misspellings tests, part 1
.if 1
.  info 1-then
.elif 1				# ok
.  info 1-elif
.elsif 1			# oops: misspelled
.  info 1-elsif
.elseif 1			# oops: misspelled
.  info 1-elseif
.endif

.info begin .elif misspellings tests, part 2
.if 0
.  info 0-then
.elif 0				# ok
.  info 0-elif
.elsif 0			# oops: misspelled
.  info 0-elsif
.elseif 0			# oops: misspelled
.  info 0-elseif
.endif

.info begin .elif misspellings tests, part 3
.if 0
.  info 0-then
.elsif 0			# oops: misspelled
.  info 0-elsif
.endif
.if 0
.  info 0-then
.elseif 0			# oops: misspelled
.  info 0-elseif
.endif

.info which branch is taken on misspelling after false?
.if 0
.  info 0-then
.elsif 1
.  info 1-elsif
.elsif 2
.  info 2-elsif
.else
.  info else
.endif

.info which branch is taken on misspelling after true?
.if 1
.  info 1-then
.elsif 1
.  info 1-elsif
.elsif 2
.  info 2-elsif
.else
.  info else
.endif

all:
	@:;
