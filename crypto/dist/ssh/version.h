/*	$NetBSD: version.h,v 1.34 2005/04/23 16:53:29 christos Exp $	*/
/* $OpenBSD: version.h,v 1.43 2005/03/08 23:49:48 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_4.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20050423"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
