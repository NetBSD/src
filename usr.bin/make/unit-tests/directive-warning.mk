# $NetBSD: directive-warning.mk,v 1.5 2020/12/13 01:10:22 rillig Exp $
#
# Tests for the .warning directive.

# TODO: Implementation

.warn				# misspelled
.warn message			# misspelled
.warnin				# misspelled
.warnin	message			# misspelled
.warning			# oops: should be "missing argument"
.warning message		# ok
.warnings			# misspelled
.warnings messages		# Accepted before 2020-12-13 01:07:54.

all:
	@:;
