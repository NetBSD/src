/*	$NetBSD: version.h,v 1.25 2002/06/24 05:48:40 itojun Exp $	*/
/* $OpenBSD: version.h,v 1.33 2002/06/21 15:41:20 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.3"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20020624"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
