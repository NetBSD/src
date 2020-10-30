# $NetBSD: cond-cmp-string.mk,v 1.7 2020/10/30 08:13:17 rillig Exp $
#
# Tests for string comparisons in .if conditions.

# This is a simple comparison of string literals.
# Nothing surprising here.
.if "str" != "str"
.  error
.endif

# The right-hand side of the comparison may be written without quotes.
.if "str" != str
.  error
.endif

# The left-hand side of the comparison must be enclosed in quotes.
# This one is not enclosed in quotes and thus generates an error message.
.if str != str
.  error
.endif

# The left-hand side of the comparison requires a defined variable.
# The variable named "" is not defined, but applying the :U modifier to it
# makes it "kind of defined" (see VAR_KEEP).  Therefore it is ok here.
.if ${:Ustr} != "str"
.  error
.endif

# Any character in a string literal may be escaped using a backslash.
# This means that "\n" does not mean a newline but a simple "n".
.if "string" != "\s\t\r\i\n\g"
.  error
.endif

# It is not possible to concatenate two string literals to form a single
# string.
.if "string" != "str""ing"
.  error
.endif

# There is no = operator for strings.
.if !("value" = "value")
.  error
.else
.  error
.endif

# There is no === operator for strings either.
.if !("value" === "value")
.  error
.else
.  error
.endif

# A variable expression can be enclosed in double quotes.
.if ${:Uword} != "${:Uword}"
.  error
.endif

# XXX: As of 2020-10-30, adding a space to the string results in a parse
# error.  This is a bug and should have been caught much earlier.
# I wonder since when it exists.
.if ${:Uword} != "${:Uword} "
.  error
.else
.  error
.endif

# Adding a space at the beginning of the quoted variable expression works
# though.
.if ${:U word } != " ${:Uword} "
.  error
.endif
