/*	$NetBSD: version.h,v 1.52 2025/10/11 15:45:08 christos Exp $	*/
/* $OpenBSD: version.h,v 1.107 2025/10/08 00:32:52 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_10.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20251011"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
#define SSH_RELEASE	SSH_VERSION
