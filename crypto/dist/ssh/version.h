/*	$NetBSD: version.h,v 1.41.6.1 2009/05/13 19:15:59 jym Exp $	*/
/* $OpenBSD: version.h,v 1.54 2008/07/21 08:19:07 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_5.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20090216"
#define SSH_HPN         "-hpn13v5"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN
