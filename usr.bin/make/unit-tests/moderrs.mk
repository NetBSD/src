# $Id: moderrs.mk,v 1.2 2020/07/26 10:04:06 rillig Exp $
#
# various modifier error tests

VAR=TheVariable
# incase we have to change it ;-)
MOD_UNKN=Z
MOD_TERM=S,V,v
MOD_S:= ${MOD_TERM},

all:	modunkn modunknV varterm vartermV modtermV modloop

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
	@echo "Expect: errors about missing @ delimiter"
	@echo ${UNDEF:U1 2 3:@var}
	@echo ${UNDEF:U1 2 3:@var@...}
	@echo ${UNDEF:U1 2 3:@var@${var}@}
