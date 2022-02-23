/*	$NetBSD: version.h,v 1.39 2022/02/23 19:07:20 christos Exp $	*/
/* $OpenBSD: version.h,v 1.93 2022/02/23 11:07:09 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.9"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20220223"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
