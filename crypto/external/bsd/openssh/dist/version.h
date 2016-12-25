/*	$NetBSD: version.h,v 1.22 2016/12/25 00:07:47 christos Exp $	*/
/* $OpenBSD: version.h,v 1.78 2016/12/19 04:55:51 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_7.4"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20160802"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
