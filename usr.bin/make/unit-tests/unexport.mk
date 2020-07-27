# $Id: unexport.mk,v 1.2 2020/07/27 19:45:56 rillig Exp $

# pick up a bunch of exported vars
FILTER_CMD=	grep ^UT_
.include "export.mk"

.unexport UT_ZOO UT_FOO

UT_TEST = unexport
