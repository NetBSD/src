/*	$NetBSD: version.h,v 1.19 2001/11/27 04:16:08 itojun Exp $	*/
/* $OpenBSD: version.h,v 1.26 2001/11/13 09:03:57 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.0.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20011127"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
