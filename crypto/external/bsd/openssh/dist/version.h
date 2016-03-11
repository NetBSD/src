/*	$NetBSD: version.h,v 1.20 2016/03/11 01:55:00 christos Exp $	*/
/* $OpenBSD: version.h,v 1.76 2016/02/23 09:14:34 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_7.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20160310"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
