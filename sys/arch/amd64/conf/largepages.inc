#	$NetBSD: largepages.inc,v 1.2.6.3 2008/01/21 09:35:22 yamt Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions 	KERN_LDSCRIPT="kern.ldscript.2MB"
