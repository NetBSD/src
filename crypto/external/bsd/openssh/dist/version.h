/*	$NetBSD: version.h,v 1.37 2021/09/02 11:26:18 christos Exp $	*/
/* $OpenBSD: version.h,v 1.91 2021/08/20 03:22:55 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.7"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20210902"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
