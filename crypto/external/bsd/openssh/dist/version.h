/*	$NetBSD: version.h,v 1.30 2020/02/27 00:24:40 christos Exp $	*/
/* $OpenBSD: version.h,v 1.86 2020/02/14 00:39:20 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-2020025"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
