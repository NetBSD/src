/*	$NetBSD: version.h,v 1.21 2002/03/08 02:00:57 itojun Exp $	*/
/* $OpenBSD: version.h,v 1.28 2002/03/06 00:25:55 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20020308"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
