# $NetBSD: directive-for-lines.mk,v 1.2 2020/12/19 12:24:46 rillig Exp $
#
# Tests for the line numbers that are reported in .for loops.
#
# Between 2007-01-01 (git 4d3c468f96e1080e, parse.c 1.127) and 2020-12-19
# (parse.c 1.494), the line numbers for the .info directives and error
# messages inside .for loops had been wrong since ParseGetLine skipped empty
# lines, even when collecting the lines for the .for loop body.

.for outer in a b

# comment

.for inner in 1 2

# comment

.info expect 18

.endfor

# comment

.info expect 24

.endfor
