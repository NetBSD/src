/* $OpenBSD: version.h,v 1.16 2001/01/08 22:29:05 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_2.3.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20010208"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
