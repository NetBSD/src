#	$NetBSD: largepages.inc,v 1.1.110.1 2007/10/25 22:35:49 bouyer Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions	KERN_LDSCRIPT="kern.ldscript.4MB"
