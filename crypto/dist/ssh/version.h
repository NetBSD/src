/*	$NetBSD: version.h,v 1.3 2000/10/28 13:41:55 itojun Exp $	*/

#define __OPENSSH_VERSION	"OpenSSH_2.2.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20001003"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment.
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
