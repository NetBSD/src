/*	$NetBSD: version.h,v 1.55 2026/10/07 17:32:09 christos Exp $	*/
/* $OpenBSD: version.h,v 1.111 2026/10/05 09:54:05 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_10.6"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20261007"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
#define SSH_RELEASE	SSH_VERSION
