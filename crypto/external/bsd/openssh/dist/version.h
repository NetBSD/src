/*	$NetBSD: version.h,v 1.35 2021/03/05 17:47:16 christos Exp $	*/
/* $OpenBSD: version.h,v 1.89 2021/03/02 01:48:18 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.5"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20210304"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
