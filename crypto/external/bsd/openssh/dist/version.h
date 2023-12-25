/*	$NetBSD: version.h,v 1.41.2.3 2023/12/25 12:22:56 martin Exp $	*/
/* $OpenBSD: version.h,v 1.100 2023/12/18 14:48:44 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_9.6"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20231220"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
