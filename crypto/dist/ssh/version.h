/*	$NetBSD: version.h,v 1.30 2003/09/16 23:17:00 christos Exp $	*/
/* $OpenBSD: version.h,v 1.39 2003/09/16 21:02:40 markus Exp $ */

#define SSH_VERSION	"OpenSSH_3.7.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20030916a"

/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	(__OPENSSH_VERSION " " __NETBSDSSH_VERSION)
