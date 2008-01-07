#	$NetBSD: largepages.inc,v 1.3 2008/01/07 08:39:07 martti Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERN_LDSCRIPT="kern.ldscript.4MB"
