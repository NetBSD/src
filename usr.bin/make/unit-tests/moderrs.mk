# $Id: moderrs.mk,v 1.5 2020/07/28 00:13:29 rillig Exp $
#
# various modifier error tests

VAR=TheVariable
# incase we have to change it ;-)
MOD_UNKN=Z
MOD_TERM=S,V,v
MOD_S:= ${MOD_TERM},

all:	modunkn modunknV varterm vartermV modtermV modloop
all:	modloop-close
all:	modwords
all:	modexclam

modunkn:
	@echo "Expect: Unknown modifier 'Z'"
	@echo "VAR:Z=${VAR:Z}"

modunknV:
	@echo "Expect: Unknown modifier 'Z'"
	@echo "VAR:${MOD_UNKN}=${VAR:${MOD_UNKN}}"

varterm:
	@echo "Expect: Unclosed variable specification for VAR"
	@echo VAR:S,V,v,=${VAR:S,V,v,

vartermV:
	@echo "Expect: Unclosed variable specification for VAR"
	@echo VAR:${MOD_TERM},=${VAR:${MOD_S}

modtermV:
	@echo "Expect: Unclosed substitution for VAR (, missing)"
	-@echo "VAR:${MOD_TERM}=${VAR:${MOD_TERM}}"

modloop:
	@echo "Expect: 2 errors about missing @ delimiter"
	@echo ${UNDEF:U1 2 3:@var}
	@echo ${UNDEF:U1 2 3:@var@...}
	@echo ${UNDEF:U1 2 3:@var@${var}@}

# The closing brace after the ${var} is part of the replacement string.
# In ParseModifierPart, braces and parentheses don't have to be balanced.
# This is contrary to the :M, :N modifiers, where both parentheses and
# braces must be balanced.
# This is also contrary to the SysV modifier, where only the actually
# used delimiter (either braces or parentheses) must be balanced.
modloop-close:
	@echo $@:
	@echo ${UNDEF:U1 2 3:@var@${var}}...@
	@echo ${UNDEF:U1 2 3:@var@${var}}...@}

modwords:
	@echo "Expect: 2 errors about missing ] delimiter"
	@echo ${UNDEF:U1 2 3:[}
	@echo ${UNDEF:U1 2 3:[#}

	# out of bounds => empty
	@echo 13=${UNDEF:U1 2 3:[13]}

	# Word index out of bounds.
	#
	# On LP64I32, strtol returns LONG_MAX,
	# which is then truncated to int (undefined behavior),
	# typically resulting in -1.
	# This -1 is interpreted as "the last word".
	#
	# On ILP32, strtol returns LONG_MAX,
	# which is a large number.
	# This results in a range from LONG_MAX - 1 to 3,
	# which is empty.
	@echo 12345=${UNDEF:U1 2 3:[123451234512345123451234512345]:S,^$,ok,:S,^3$,ok,}

modexclam:
	@echo "Expect: 2 errors about missing ! delimiter"
	@echo ${VARNAME:!echo}
	# When the final exclamation mark is missing, there is no
	# fallback to the SysV substitution modifier.
	# If there were a fallback, the output would be "exclam",
	# and the above would have produced an "Unknown modifier '!'".
	@echo ${!:L:!=exclam}
