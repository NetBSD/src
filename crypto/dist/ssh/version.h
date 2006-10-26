/*	$NetBSD: version.h,v 1.33.2.1 2006/10/26 09:39:57 ghen Exp $	*/
/* $OpenBSD: version.h,v 1.42 2004/08/16 08:17:01 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.9"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20061016"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
