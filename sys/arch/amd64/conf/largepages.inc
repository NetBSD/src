#	$NetBSD: largepages.inc,v 1.1.4.1 2007/10/25 22:35:33 bouyer Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions	KERN_LDSCRIPT="kern.ldscript.2MB"
