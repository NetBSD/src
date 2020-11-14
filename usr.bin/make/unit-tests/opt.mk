# $NetBSD: opt.mk,v 1.3 2020/11/14 17:33:51 rillig Exp $
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
	# standard getopt programs.  Yes, it does, and it treats the '-f' as
	# a target to be created.
	${MAKE} -- -VAR=value -f /dev/null
	@echo

	# This is the normal way to print the usage of a command.
	${MAKE} -?
	@echo
