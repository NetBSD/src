#	$NetBSD: largepages.inc,v 1.2.2.1 2008/02/18 21:04:37 mjf Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERN_LDSCRIPT="kern.ldscript.4MB"
