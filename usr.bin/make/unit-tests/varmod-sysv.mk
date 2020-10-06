# $NetBSD: varmod-sysv.mk,v 1.4 2020/10/06 21:05:21 rillig Exp $
#
# Tests for the ${VAR:from=to} variable modifier, which replaces the suffix
# "from" with "to".  It can also use '%' as a wildcard.
#
# This modifier is applied when the other modifiers don't match exactly.

all:

# The :Q looks like a modifier but isn't.
# It is part of the replacement string.
.if ${a b c d e:L:%a=x:Q} != "x:Q b c d e"
.  error
.endif

# Before 2020-07-19, an ampersand could be used in the replacement part
# of a SysV substitution modifier, and it was replaced with the whole match,
# just like in the :S modifier.
#
# This was probably a copy-and-paste mistake since the code for the SysV
# modifier looked a lot like the code for the :S and :C modifiers.
# The ampersand is not mentioned in the manual page.
.if ${a.bcd.e:L:a.%=%} != "bcd.e"
.  error
.endif
# Before 2020-07-19, the result of the expression was "a.bcd.e".
.if ${a.bcd.e:L:a.%=&} != "&"
.  error
.endif

# Before 2020-07-20, when a SysV modifier was parsed, a single dollar
# before the '=' was parsed (but not interpreted) as an anchor.
# Parsing something without then evaluating it accordingly doesn't make
# sense.
.if ${value:L:e$=x} != "value"
.  error
.endif
# Before 2020-07-20, the modifier ":e$=x" was parsed as having a left-hand
# side "e" and a right-hand side "x".  The dollar was parsed (but not
# interpreted) as 'anchor at the end'.  Therefore the modifier was equivalent
# to ":e=x", which doesn't match the string "value$".  Therefore the whole
# expression evaluated to "value$".
.if ${${:Uvalue\$}:L:e$=x} != "valux"
.  error
.endif
.if ${value:L:e=x} != "valux"
.  error
.endif

# Words that don't match are copied unmodified.
.if ${:Ufile.c file.h:%.c=%.cpp} != "file.cpp file.h"
.  error
.endif

# The % placeholder can be anywhere in the string, it doesn't have to be at
# the beginning of the pattern.
.if ${:Ufile.c other.c:file.%=renamed.%} != "renamed.c other.c"
.  error
.endif

# Trying to cover all possible variants of the SysV modifier.
LIST=	one two

.if ${LIST:o=X} != "one twX"
.  error
.endif
.if ${LIST:o=} != "one tw"
.  error
.endif
.if ${LIST:o=%} != "one tw%"
.  error
.endif
.if ${LIST:%o=X} != "one X"
.  error
.endif
.if ${LIST:o%=X} != "X two"
.  error
.endif
.if ${LIST:o%e=X} != "X two"
.  error
.endif
.if ${LIST:o%%e=X} != "one two"	# Only the first '%' is the wildcard.
.  error
.endif
.if ${LIST:%=%%} != "one% two%"	# None of the words contains a literal '%'.
.  error
.endif
.if ${LIST:%nes=%xxx} != "one two" # lhs is longer than the word "one"
.  error
.endif
