/* $OpenBSD: version.h,v 1.19 2001/02/19 10:35:23 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_2.5.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20010219"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
