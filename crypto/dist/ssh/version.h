/* $OpenBSD: version.h,v 1.20 2001/03/19 17:12:10 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_2.5.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20010319"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
