# From spcecdt@armory.com  Tue Jun  3 11:41:49 2003
# Return-Path: <spcecdt@armory.com>
# Received: from localhost (skeeve [127.0.0.1])
# 	by skeeve.com (8.12.5/8.12.5) with ESMTP id h538fVuW018115
# 	for <arnold@localhost>; Tue, 3 Jun 2003 11:41:49 +0300
# Received: from actcom.co.il [192.114.47.1]
# 	by localhost with POP3 (fetchmail-5.9.0)
# 	for arnold@localhost (single-drop); Tue, 03 Jun 2003 11:41:49 +0300 (IDT)
# Received: by actcom.co.il (mbox arobbins)
#  (with Cubic Circle's cucipop (v1.31 1998/05/13) Tue Jun  3 11:41:37 2003)
# X-From_: spcecdt@armory.com Mon Jun  2 20:17:30 2003
# Received: from smtp1.actcom.net.il by actcom.co.il  with ESMTP
# 	(8.11.6/actcom-0.2) id h52HHNY23516 for <arobbins@actcom.co.il>;
# 	Mon, 2 Jun 2003 20:17:24 +0300 (EET DST)  
# 	(rfc931-sender: mail.actcom.co.il [192.114.47.13])
# Received: from f7.net (consort.superb.net [209.61.216.22])
# 	by smtp1.actcom.net.il (8.12.8/8.12.8) with ESMTP id h52HIHqv028728
# 	for <arobbins@actcom.co.il>; Mon, 2 Jun 2003 20:18:18 +0300
# Received: from armory.com (deepthought.armory.com [192.122.209.42])
# 	by f7.net (8.11.7/8.11.6) with SMTP id h52HHKl31637
# 	for <arnold@skeeve.com>; Mon, 2 Jun 2003 13:17:20 -0400
# Date: Mon, 2 Jun 2003 10:17:11 -0700
# From: "John H. DuBois III" <spcecdt@armory.com>
# To: arnold@skeeve.com
# Subject: gawk 3.1.2c coredump
# Message-ID: <20030602171711.GA3958@armory.com>
# Mime-Version: 1.0
# Content-Type: text/plain; charset=us-ascii
# Content-Disposition: inline
# User-Agent: Mutt/1.3.28i
# X-Www: http://www.armory.com./~spcecdt/
# Sender: spcecdt@armory.com
# X-SpamBouncer: 1.4 (10/07/01)
# X-SBClass: OK
# Status: R
# 
#!/usr/local/bin/gawk -f
BEGIN {
    switch (substr("x",1,1)) {
    case /ask.com/:
	break
    case "google":
	break
    }
}
# 
# The stack says:
# 
# #0  0x0806fac2 in r_tree_eval (tree=0x8092000, iscond=0) at eval.c:813
# #1  0x08070413 in r_tree_eval (tree=0x0, iscond=0) at eval.c:1071
# #2  0x08070413 in r_tree_eval (tree=0x8092000, iscond=0) at eval.c:1071
# #3  0x08070413 in r_tree_eval (tree=0x8092000, iscond=0) at eval.c:1071
# [ this continues on indefinitely - I suppose it ran out of stack eventually ]
# 
# 	John
# -- 
# John DuBois  spcecdt@armory.com  KC6QKZ/AE  http://www.armory.com/~spcecdt/
# 
# 
