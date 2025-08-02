/*	$NetBSD: version.h,v 1.45.2.1 2025/08/02 05:18:49 perseant Exp $	*/
/* $OpenBSD: version.h,v 1.105 2025/04/09 07:00:21 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_10.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20250409"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
#define SSH_RELEASE	SSH_VERSION
