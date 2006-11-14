/*	$NetBSD: version.h,v 1.37 2006/11/14 21:52:09 adrianp Exp $	*/
/* $OpenBSD: version.h,v 1.47 2006/08/30 00:14:37 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_4.4"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20061114"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
