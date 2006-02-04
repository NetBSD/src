/*	$NetBSD: version.h,v 1.35 2006/02/04 22:32:14 christos Exp $	*/
/* $OpenBSD: version.h,v 1.46 2006/02/01 11:27:22 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_4.3"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20060204"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
