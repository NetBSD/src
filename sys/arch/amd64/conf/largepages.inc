#	$NetBSD: largepages.inc,v 1.3 2008/01/07 08:30:03 martti Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERN_LDSCRIPT="kern.ldscript.2MB"
