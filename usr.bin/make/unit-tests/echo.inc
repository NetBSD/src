# Use Echo -n to work around shell builtins that do not support -n
x!= echo -n ok; echo
.if ${x:M-n} == ""
ECHO_SCRIPT= Echo() { echo "$$@"; }
.else
ECHO_SCRIPT= Echo() { \
	case "$$1" in \
	-n) shift; echo "$$@\c";; \
	*) echo "$$@";; \
	esac; }
.endif
