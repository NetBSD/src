divert(-1)
#
# Copyright (c) 1999 Sendmail, Inc. and its suppliers.
#	All rights reserved.
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the sendmail distribution.
#
#
#  Definitions for Makefile construction for sendmail
#
#	$Id: links.m4,v 1.1.1.1 2000/05/03 09:27:18 itojun Exp $
#
divert(0)dnl
define(`bldMAKE_SOURCE_LINK',
`$1: ${SRCDIR}/$1
	-ln -s ${SRCDIR}/$1 $1'
)dnl
define(`bldMAKE_SOURCE_LINKS', 
`bldFOREACH(`bldMAKE_SOURCE_LINK(', $1)'dnl
)dnl
define(`bldMAKE_TARGET_LINKS', 
`	for i in $2; do \
		rm -f $$i; \
		ln -s $1 $$i; \
	done'
)dnl

