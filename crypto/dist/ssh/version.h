/*	$NetBSD: version.h,v 1.17 2001/11/07 06:26:49 itojun Exp $	*/
/* $OpenBSD: version.h,v 1.25 2001/10/15 16:10:50 deraadt Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20011107"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
