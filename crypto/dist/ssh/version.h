/*	$NetBSD: version.h,v 1.27 2002/10/01 14:07:49 itojun Exp $	*/
/* $OpenBSD: version.h,v 1.35 2002/10/01 13:24:50 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.5"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20021001"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
