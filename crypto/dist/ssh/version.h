/* $OpenBSD: version.h,v 1.18 2001/02/16 14:26:57 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_2.5.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20010217"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
