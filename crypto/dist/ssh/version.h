/*	$NetBSD: version.h,v 1.22 2002/04/22 07:59:49 itojun Exp $	*/
/* $OpenBSD: version.h,v 1.29 2002/04/10 08:56:01 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20020422"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
