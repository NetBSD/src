/*	$NetBSD: version.h,v 1.41 2008/06/22 15:42:51 christos Exp $	*/
/* $OpenBSD: version.h,v 1.53 2008/04/03 09:50:14 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_5.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20080403"
#define SSH_HPN         "-hpn13v1"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN
