/*	$NetBSD: version.h,v 1.26 2002/06/26 14:08:34 itojun Exp $	*/
/* $OpenBSD: version.h,v 1.34 2002/06/26 13:56:27 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.4"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20020626"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
