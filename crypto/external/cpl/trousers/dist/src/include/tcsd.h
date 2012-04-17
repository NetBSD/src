
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */


#ifndef _TCSD_H_
#define _TCSD_H_

#include <signal.h>

#include "rpc_tcstp.h"

/* Platform Class structures */
struct platform_class
{
	unsigned int simpleID;	/* Platform specific spec identifier */
	unsigned int classURISize;	/* Size of the classURI */
	char *classURI;	/* Specific spec. Can be NULL */
	struct platform_class *next;
};

/* config structures */
struct tcsd_config
{
	int port;		/* port the TCSD will listen on */
	unsigned int num_threads;	/* max number of threads the TCSD allows simultaneously */
	char *system_ps_dir;	/* the directory the system PS file sits in */
	char *system_ps_file;	/* the name of the system PS file */
	char *firmware_log_file;/* the name of the firmware PCR event file */
	char *kernel_log_file;	/* the name of the kernel PCR event file */
	unsigned int kernel_pcrs;	/* bitmask of PCRs the kernel controls */
	unsigned int firmware_pcrs;	/* bitmask of PCRs the firmware controls */
	char *platform_cred;		/* location of the platform credential */
	char *conformance_cred;		/* location of the conformance credential */
	char *endorsement_cred;		/* location of the endorsement credential */
	int remote_ops[TCSD_MAX_NUM_ORDS];	/* array of ordinals executable by remote hosts */
	unsigned int unset;	/* bitmask of options which are still unset */
	int exclusive_transport; /* allow applications to open exclusive transport sessions with
				    the TPM and enforce their exclusivity (possible DOS issue) */
	struct platform_class *host_platform_class; /* Host platform class of this TCS System */
	struct platform_class *all_platform_classes;	/* List of platform classes
							of this TCS System */
};

#define TCSD_DEFAULT_CONFIG_FILE	ETC_PREFIX "/tcsd.conf"
extern char *tcsd_config_file;

#ifdef __NetBSD__
#define TSS_USER_NAME		"_tss"
#define TSS_GROUP_NAME		"_tss"
#else
#define TSS_USER_NAME		"tss"
#define TSS_GROUP_NAME		"tss"
#endif

#define TCSD_DEFAULT_MAX_THREADS	10
#define TCSD_DEFAULT_SYSTEM_PS_FILE	VAR_PREFIX "/lib/tpm/system.data"
#define TCSD_DEFAULT_SYSTEM_PS_DIR	VAR_PREFIX "/lib/tpm"
#define TCSD_DEFAULT_FIRMWARE_LOG_FILE	"/sys/kernel/security/tpm0/binary_bios_measurements"
#define TCSD_DEFAULT_KERNEL_LOG_FILE	"/sys/kernel/security/ima/binary_runtime_measurements"
#define TCSD_DEFAULT_FIRMWARE_PCRS	0x00000000
#define TCSD_DEFAULT_KERNEL_PCRS	0x00000000

/* This will change when a system with more than 32 PCR's exists */
#define TCSD_MAX_PCRS			32

/* this is the 2nd param passed to the listen() system call */
#define TCSD_MAX_SOCKETS_QUEUED		50
#define TCSD_TXBUF_SIZE			1024

/* The Available Tcs Platform Classes */
struct tcg_platform_spec {
	char *name;
	TPM_PLATFORM_SPECIFIC specNo;
	char *specURI;
};

/* The Specific URI's for the platforms specs on TCG website */
#define TPM_PS_PC_11_URI	"https://www.trustedcomputinggroup.org/groups/pc_client/TCG_PCSpecificSpecification_v1_1.pdf"
#define TPM_PS_PC_12_URI	"https://www.trustedcomputinggroup.org/specs/PCClient/TCG_PCClientImplementationforBIOS_1-20_1-00.pdf"
#define TPM_PS_PDA_12_URI	"https://www.trustedcomputinggroup.org/specs/mobilephone/tcg-mobile-reference-architecture-1.0.pdf"
#define TPM_PS_Server_12_URI	"https://www.trustedcomputinggroup.org/specs/Server/TCG_Generic_Server_Specification_v1_0_rev0_8.pdf"
#define TPM_PS_Mobile_12_URI	"https://www.trustedcomputinggroup.org/specs/mobilephone/tcg-mobile-reference-architecture-1.0.pdf"

/* for detecting whether an option has been set */
#define TCSD_OPTION_PORT		0x0001
#define TCSD_OPTION_MAX_THREADS		0x0002
#define TCSD_OPTION_FIRMWARE_PCRS	0x0004
#define TCSD_OPTION_KERNEL_PCRS		0x0008
#define TCSD_OPTION_SYSTEM_PSFILE	0x0010
#define TCSD_OPTION_KERNEL_LOGFILE	0x0020
#define TCSD_OPTION_FIRMWARE_LOGFILE	0x0040
#define TCSD_OPTION_PLATFORM_CRED	0x0080
#define TCSD_OPTION_CONFORMANCE_CRED	0x0100
#define TCSD_OPTION_ENDORSEMENT_CRED	0x0200
#define TCSD_OPTION_REMOTE_OPS		0x0400
#define TCSD_OPTION_EXCLUSIVE_TRANSPORT	0x0800
#define TCSD_OPTION_HOST_PLATFORM_CLASS	0x1000

#define TSS_TCP_RPC_MAX_DATA_LEN	1048576
#define TSS_TCP_RPC_BAD_PACKET_TYPE	0x10000000

enum tcsd_config_option_code {
	opt_port = 1,
	opt_max_threads,
	opt_system_ps_file,
	opt_firmware_log,
	opt_kernel_log,
	opt_firmware_pcrs,
	opt_kernel_pcrs,
	opt_platform_cred,
	opt_conformance_cred,
	opt_endorsement_cred,
	opt_remote_ops,
	opt_exclusive_transport,
	opt_host_platform_class,
	opt_all_platform_classes
};

struct tcsd_config_options {
	char *name;
	enum tcsd_config_option_code option;
};

extern struct tcsd_config tcsd_options;

TSS_RESULT conf_file_init(struct tcsd_config *);
void	   conf_file_final(struct tcsd_config *);
TSS_RESULT ps_dirs_init();
void	   tcsd_signal_handler(int);

/* threading structures */
struct tcsd_thread_data
{
	int sock;
	UINT32 context;
	THREAD_TYPE *thread_id;
	char *hostname;
	struct tcsd_comm_data comm;
};

struct tcsd_thread_mgr
{
	MUTEX_DECLARE(lock);
	struct tcsd_thread_data *thread_data;

	int shutdown;
	UINT32 num_active_threads;
	UINT32 max_threads;
};

TSS_RESULT tcsd_threads_init();
TSS_RESULT tcsd_threads_final();
TSS_RESULT tcsd_thread_create(int, char *);
void	   *tcsd_thread_run(void *);
void	   thread_signal_init();

/* signal handling */
struct sigaction tcsd_sa_int;
struct sigaction tcsd_sa_chld;

#endif
