/*	$NetBSD: version.h,v 1.4 2001/01/14 05:22:33 itojun Exp $	*/

#define __OPENSSH_VERSION	"OpenSSH_2.3.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20010114"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment.
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
