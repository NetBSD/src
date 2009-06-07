/*	$NetBSD: version.h,v 1.2 2009/06/07 22:38:48 christos Exp $	*/
/* $OpenBSD: version.h,v 1.55 2009/02/23 00:06:15 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_5.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20090605"
#define SSH_HPN         "-hpn13v6"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN
