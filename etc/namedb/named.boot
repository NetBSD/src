;	$NetBSD: named.boot,v 1.4 1998/09/14 05:05:44 marc Exp $
;	from @(#)named.boot	8.1 (Berkeley) 6/9/93

; boot file for secondary name server
; Note that there should be one primary entry for each SOA record.

directory	/etc/namedb

; type    domain		source host/file		backup file

cache     .							root.cache
primary   0.0.127.IN-ADDR.ARPA	localhost.rev

; example secondary server config:
; secondary Berkeley.EDU	128.32.130.11 128.32.133.1	ucbhosts.bak
; secondary 32.128.IN-ADDR.ARPA	128.32.130.11 128.32.133.1	ucbhosts.rev.bak

; example primary server config:
; primary  Berkeley.EDU		ucbhosts
; primary  32.128.IN-ADDR.ARPA	ucbhosts.rev
