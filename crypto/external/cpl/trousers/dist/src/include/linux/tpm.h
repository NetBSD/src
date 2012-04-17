
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */

/*
 * include/linux/tpm.h
 *
 * Device driver for TCPA TPM (trusted platform module).
 */
#ifndef _TPM_H_
#define _TPM_H_

#if (defined (__linux) || defined (linux))
#include <linux/ioctl.h>
#elif (defined (__OpenBSD__) || defined (__FreeBSD__) || defined(__NetBSD__))
#include <sys/ioctl.h>
#endif

/* ioctl commands */
#define	TPMIOC_CANCEL		_IO('T', 0x00)
#define	TPMIOC_TRANSMIT		_IO('T', 0x01)

#if defined(__KERNEL__)
extern ssize_t tpm_transmit(const char *buf, size_t bufsiz);
extern ssize_t tpm_extend(int index, u8 *digest);
extern ssize_t tpm_pcrread(int index, u8 *hash);
extern ssize_t tpm_dirread(int index, u8 *hash);
extern ssize_t tpm_cap_version(int *maj, int *min, int *ver, int *rev);
extern ssize_t tpm_cap_pcr(int *pcrs);
extern ssize_t tpm_cap_dir(int *dirs);
extern ssize_t tpm_cap_manufacturer(int *manufacturer);
extern ssize_t tpm_cap_slot(int *slots);
#endif /* __KERNEL__ */

#endif
