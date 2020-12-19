# $NetBSD: directive-info.mk,v 1.7 2020/12/19 12:40:00 rillig Exp $
#
# Tests for the .info directive.

# TODO: Implementation

.info begin .info tests
.inf				# misspelled
.info				# oops: message should be "missing parameter"
.info message
.info		indented message
.information
.information message		# Accepted before 2020-12-13 01:07:54.
.info.man:			# not a message, but possibly a suffix rule

# Even if lines would have trailing whitespace, this would be trimmed by
# ParseGetLine.
.info
.info				# comment

.info: message			# This is a dependency declaration.
.info-message			# This is an unknown directive.
.info no-target: no-source	# This is a .info directive, not a dependency.
# See directive.mk for more tests of this kind.

# Since at least 2002-01-01, the line number that is used in error messages
# and the .info directives is the number of completely read lines.  For the
# following multi-line directive, this means that the reported line number is
# the one of the last line, not the first line.
.info expect line 30 for\
	multi$\
	-line message

all:
	@:;
