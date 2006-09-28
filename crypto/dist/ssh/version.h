/*	$NetBSD: version.h,v 1.36 2006/09/28 21:22:15 christos Exp $	*/
/* $OpenBSD: version.h,v 1.47 2006/08/30 00:14:37 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_4.4"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20060928"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
