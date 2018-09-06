/*	$NetBSD: version.h,v 1.24.2.2 2018/09/06 06:51:34 pgoyette Exp $	*/
/* $OpenBSD: version.h,v 1.82 2018/07/03 11:42:12 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_7.8"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20180825"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
