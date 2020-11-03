# $NetBSD: directive-info.mk,v 1.3 2020/11/03 17:17:31 rillig Exp $
#
# Tests for the .info directive.

# TODO: Implementation

.info begin .info tests
.inf				# misspelled
.info				# oops: message should be "missing parameter"
.info message
.info		indented message
.information
.information message		# oops: misspelled
.info.man:			# not a message, but possibly a suffix rule

all:
	@:;
