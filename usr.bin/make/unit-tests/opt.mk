# $NetBSD: opt.mk,v 1.4 2020/11/15 06:06:19 sjg Exp $
#
# Tests for the command line options.

# TODO: Implementation

.MAKEFLAGS: -d0			# make stdout line-buffered

all: .IGNORE
	# Just to see how the custom argument parsing code reacts to a syntax
	# error.  The colon is used in the options string, marking an option
	# that takes arguments.  It is not an option by itself, though.
	${MAKE} -:
	@echo

	# See whether a '--' stops handling of command line options, like in
	# standard getopt programs.  Yes, it does, and it treats the
	# second '-f' as a target to be created.
	${MAKE} -r -f /dev/null -- -VAR=value -f /dev/null
	@echo

	# This is the normal way to print the usage of a command.
	${MAKE} -?
	@echo
