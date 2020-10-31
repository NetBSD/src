# $NetBSD: varmod-sysv.mk,v 1.6 2020/10/31 08:31:37 rillig Exp $
#
# Tests for the ${VAR:from=to} variable modifier, which replaces the suffix
# "from" with "to".  It can also use '%' as a wildcard.
#
# This modifier is applied when the other modifiers don't match exactly.
#
# See ApplyModifier_SysV.

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

# The suffix "o" is replaced with "X".
.if ${one two:L:o=X} != "one twX"
.  error
.endif

# The suffix "o" is replaced with nothing.
.if ${one two:L:o=} != "one tw"
.  error
.endif

# The suffix "o" is replaced with a literal percent.  The percent is only
# a wildcard when it appears on the left-hand side.
.if ${one two:L:o=%} != "one tw%"
.  error
.endif

# Each word with the suffix "o" is replaced with "X".  The percent is a
# wildcard even though the right-hand side does not contain another percent.
.if ${one two:L:%o=X} != "one X"
.  error
.endif

# Each word with the prefix "o" is replaced with "X".  The percent is a
# wildcard even though the right-hand side does not contain another percent.
.if ${one two:L:o%=X} != "X two"
.  error
.endif

# For each word with the prefix "o" and the suffix "e", the whole word is
# replaced with "X".
.if ${one two oe oxen:L:o%e=X} != "X two X oxen"
.  error
.endif

# Only the first '%' is the wildcard.
.if ${one two o%e other%e:L:o%%e=X} != "one two X X"
.  error
.endif

# In the replacement, only the first '%' is the placeholder, all others
# are literal percent characters.
.if ${one two:L:%=%%} != "one% two%"
.  error
.endif

# In the word "one", only a prefix of the pattern suffix "nes" matches,
# the whole word is too short.  Therefore it doesn't match.
.if ${one two:L:%nes=%xxx} != "one two"
.  error
.endif

# As of 2020-10-06, the right-hand side of the SysV modifier is expanded
# twice.  The first expansion happens in ApplyModifier_SysV, where the
# modifier is split into its two parts.  The second expansion happens
# when each word is replaced in ModifyWord_SYSVSubst.
# XXX: This is unexpected.  Add more test case to demonstrate the effects
# of removing one of the expansions.
VALUE=		value
INDIRECT=	1:${VALUE} 2:$${VALUE} 4:$$$${VALUE}
.if ${x:L:x=${INDIRECT}} != "1:value 2:value 4:\${VALUE}"
.  error
.endif
