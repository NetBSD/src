;	$NetBSD: named.boot,v 1.5 1999/01/22 01:41:19 mycroft Exp $
;	from @(#)named.boot	8.1 (Berkeley) 6/9/93

; boot file for secondary name server
; Note that there should be one primary entry for each SOA record.

directory	/etc/namedb

; type    domain		source host/file		backup file

cache     .							root.cache
primary   127.IN-ADDR.ARPA	127

; example secondary server config:
; secondary Berkeley.EDU	128.32.130.11 128.32.133.1	berkeley.edu.cache
; secondary 32.128.IN-ADDR.ARPA	128.32.130.11 128.32.133.1	128.32.bak

; example primary server config:
; primary  Berkeley.EDU		berkeley.edu
; primary  32.128.IN-ADDR.ARPA	128.32
