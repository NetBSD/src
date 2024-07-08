/*	$NetBSD: version.h,v 1.47 2024/07/08 22:33:44 christos Exp $	*/
/* $OpenBSD: version.h,v 1.102 2024/07/01 04:31:59 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_9.8"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20240704"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
