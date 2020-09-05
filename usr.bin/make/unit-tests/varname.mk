# $NetBSD: varname.mk,v 1.3 2020/09/05 12:59:07 rillig Exp $
#
# Tests for special variables, such as .MAKE or .PARSEDIR.

# These following MAGIC variables have the same hash code, at least with
# the default hashing algorithm, which is the same as in Java.  The order
# in which these variables are defined determines the order in which they
# appear in the Hash_Table.  New entries are prepended to the bucket lists,
# therefore this test numbers the values in descending order.

.if defined(ORDER_01)

MAGIC0a0a0a=	8
MAGIC0a0a1B=	7
MAGIC0a1B0a=	6
MAGIC0a1B1B=	5
MAGIC1B0a0a=	4
MAGIC1B0a1B=	3
MAGIC1B1B0a=	2
MAGIC1B1B1B=	1

all: # nothing

.elif defined(ORDER_10)

MAGIC1B1B1B=	8
MAGIC1B1B0a=	7
MAGIC1B0a1B=	6
MAGIC1B0a0a=	5
MAGIC0a1B1B=	4
MAGIC0a1B0a=	3
MAGIC0a0a1B=	2
MAGIC0a0a0a=	1

all: # nothing

.else

all:
	@${.MAKE} -f ${MAKEFILE} -dg1 ORDER_01=yes
	@${.MAKE} -f ${MAKEFILE} -dg1 ORDER_10=yes

.endif
