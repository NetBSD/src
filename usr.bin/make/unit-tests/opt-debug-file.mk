# $NetBSD: opt-debug-file.mk,v 1.6 2021/12/28 01:04:04 rillig Exp $
#
# Tests for the -dF command line option, which redirects the debug log
# to a file instead of writing it to stderr.

# Enable debug logging for variable assignments and evaluation (-dv)
# and redirect the debug logging to the given file.
.MAKEFLAGS: -dvFopt-debug-file.debuglog

# This output goes to the debug log file.
VAR=	value ${:Uexpanded}

# Hide the logging output for the remaining actions.
# As of 2020-10-03, it is not possible to disable debug logging again.
.MAKEFLAGS: -dF/dev/null

# Make sure that the debug logging file contains some logging.
DEBUG_OUTPUT:=	${:!cat opt-debug-file.debuglog!}
# Grmbl.  Because of the := operator in the above line, the variable
# value contains ${:Uexpanded}.  This variable expression is expanded
# upon further processing.  Therefore, don't read from untrusted input.
#.MAKEFLAGS: -dc -dFstderr
.if !${DEBUG_OUTPUT:tW:M*VAR = value expanded*}
.  error ${DEBUG_OUTPUT}
.endif

# To get the unexpanded text that was actually written to the debug log
# file, the content of that log file must not be stored in a variable.
# XXX: In the :M modifier, a dollar is escaped as '$$', not '\$'.
.if !${:!cat opt-debug-file.debuglog!:tW:M*VAR = value $${:Uexpanded}*}
.  error
.endif


# If the debug log file cannot be opened, make prints an error message and
# exits immediately since the debug log file is usually selected from the
# command line.
.MAKEFLAGS: -dFopt-debug-file.debuglog/file

all:
