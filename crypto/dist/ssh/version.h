/*	$NetBSD: version.h,v 1.39 2007/12/18 02:35:33 christos Exp $	*/
/* $OpenBSD: version.h,v 1.50 2007/08/15 08:16:49 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_4.7"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20071217"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
