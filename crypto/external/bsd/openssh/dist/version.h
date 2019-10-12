/*	$NetBSD: version.h,v 1.29 2019/10/12 18:32:22 christos Exp $	*/
/* $OpenBSD: version.h,v 1.85 2019/10/09 00:04:57 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_8.1"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20191009"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
