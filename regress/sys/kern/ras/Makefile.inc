#	$NetBSD: Makefile.inc,v 1.1 2004/01/06 21:40:57 martin Exp $

# Avoid optimizations that move common code out of the RAS.
# The compiler does not understand RAS, so it makes wrong assumptions
# about the lifetime of register values.
DBG += -O0
