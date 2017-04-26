/*	$NetBSD: version.h,v 1.20.2.3 2017/04/26 02:52:15 pgoyette Exp $	*/
/* $OpenBSD: version.h,v 1.79 2017/03/20 01:18:59 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_7.5"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20170418"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
