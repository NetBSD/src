/* $OpenBSD: version.h,v 1.17 2001/02/08 18:15:22 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_2.3.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20010209"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
