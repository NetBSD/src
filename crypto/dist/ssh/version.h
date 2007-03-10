/*	$NetBSD: version.h,v 1.38 2007/03/10 22:52:10 christos Exp $	*/
/* $OpenBSD: version.h,v 1.49 2007/03/06 10:13:14 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_4.6"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20070310"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
