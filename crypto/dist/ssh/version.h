/*	$NetBSD: version.h,v 1.16 2001/09/27 03:24:07 itojun Exp $	*/
/* $OpenBSD: version.h,v 1.24 2001/09/24 20:31:54 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_2.9.9"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20010927"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
