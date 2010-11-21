/*	$NetBSD: version.h,v 1.5 2010/11/21 18:59:04 adam Exp $	*/
/* $OpenBSD: version.h,v 1.59 2010/08/08 16:26:42 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_5.6"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20091226"
#define SSH_HPN         "-hpn13v10"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
