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
#	$Id: subst_ext.m4,v 1.1.1.1 2000/05/03 09:27:17 itojun Exp $
#
divert(0)dnl
define(`bldSUBST_EXTENSION',
`substr($2, 0, bldRINDEX($2, `.'))`'.$1 'dnl
)dnl
define(`bldSUBST_EXTENSIONS',
`bldFOREACH(`bldSUBST_EXTENSION(`$1',', $2)'dnl
)dnl
define(`bldREMOVE_COMMAS',
`$1 ifelse($#, 1, , `bldREMOVE_COMMAS(shift($@))')'dnl
)dnl

define(`bldADD_EXTENSION', `$2.$1 ')dnl
define(`bldADD_EXTENSIONS',
`bldFOREACH(`bldADD_EXTENSION(`$1',', $2)'dnl
)dnl

