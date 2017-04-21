/*	$NetBSD: version.h,v 1.22.2.1 2017/04/21 16:50:57 bouyer Exp $	*/
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
