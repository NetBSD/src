/*	$NetBSD: version.h,v 1.1.1.1.2.12 2003/09/16 23:32:23 christos Exp $	*/
/* $OpenBSD: version.h,v 1.27 2001/12/05 15:04:48 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_3.0.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20030916a"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
