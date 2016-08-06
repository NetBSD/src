/*	$NetBSD: version.h,v 1.20.2.1 2016/08/06 00:18:39 pgoyette Exp $	*/
/* $OpenBSD: version.h,v 1.77 2016/07/24 11:45:36 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_7.3"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20160802"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
