/*	$NetBSD: version.h,v 1.42 2023/07/26 17:58:16 christos Exp $	*/
/* $OpenBSD: version.h,v 1.97 2023/03/15 21:19:57 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_9.3"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20230725"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION SSH_HPN SSH_LPK
