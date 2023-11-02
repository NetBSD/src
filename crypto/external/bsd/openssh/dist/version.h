/*	$NetBSD: version.h,v 1.41.2.2 2023/11/02 22:15:22 sborrill Exp $	*/
/* $OpenBSD: version.h,v 1.99 2023/10/04 04:04:09 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_9.5"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20231025"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
