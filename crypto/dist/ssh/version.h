/*	$NetBSD: version.h,v 1.23.2.2 2002/06/26 16:54:41 tv Exp $	*/
/* $OpenBSD: version.h,v 1.34 2002/06/26 13:56:27 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.4"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20020626"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
