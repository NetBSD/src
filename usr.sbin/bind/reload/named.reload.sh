#!/bin/sh -
#	$NetBSD: named.reload.sh,v 1.1.1.1 1998/10/05 18:02:00 tron Exp $
#
#	from named.reload	5.2 (Berkeley) 6/27/89
#	from: Id: named.reload.sh,v 8.1 1994/12/15 06:24:14 vixie Exp 
#

exec %DESTSBIN%/%INDOT%ndc reload
