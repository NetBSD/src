# $NetBSD: directive-for-lines.mk,v 1.1 2020/12/19 12:14:59 rillig Exp $
#
# Tests for the line numbers that are reported in .for loops.
#
# Since 2007-01-01 21:47:32 (git 4d3c468f96e1080e4d3cca8cc39067a73af14fbb),
# parse.c 1.127, the line numbers for the .info directives and error messages
# inside .for loops have been wrong since ParseGetLine skips empty lines, even
# when collecting the lines for the .for loop body.

.for outer in a b

# comment

.for inner in 1 2

# comment

.info expect 18

.endfor

# comment

.info expect 24

.endfor
