/*	$NetBSD: version.h,v 1.12 2001/04/10 08:08:05 itojun Exp $	*/
/* $OpenBSD: version.h,v 1.22 2001/04/05 10:39:48 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_2.5.4"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20010410"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
