#!/bin/sh -
#	$NetBSD: named.reload.sh,v 1.3 1999/11/30 20:34:51 veego Exp $
#
#	from named.reload	5.2 (Berkeley) 6/27/89
#	from: Id: named.reload.sh,v 8.1 1994/12/15 06:24:14 vixie Exp 
#

exec %DESTSBIN%/%INDOT%ndc reload
