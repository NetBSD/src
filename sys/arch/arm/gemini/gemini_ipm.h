/*	$NetBSD: gemini_ipm.h,v 1.1.32.1 2017/12/03 11:35:53 jdolecek Exp $	*/

#ifndef _GEMINI_IPM_H_
#define _GEMINI_IPM_H_

/* 
 * generic/non-specific Messages
 */
#define IPM_TAG_NONE		0
#define IPM_TAG_GPN		1
#define IPM_TAG_WDOG		2
#define IPM_NTAGS		(IPM_TAG_WDOG + 1)	/* bump when you add new ones */

typedef struct ipm_desc {
	uint8_t	tag;
	uint8_t	blob[15];
} ipm_desc_t;


/*
 * void *gemini_ipm_register(uint8_t tag, unsigned int ipl, size_t quota,
 *	void (*consume)(void *arg, const void *desc),
 *	void (*counter)(void *arg, size_t n),
 *	void *arg);
 *
 * - register an IPM service, identified by 'tag'
 * - callback functions may be dispatched using softint at indicated 'ipl'
 *   using softint_establish(), softint_disestablish()
 *   for now they are called directly at IPL_NET
 * - reserve 'quota' descriptors; minimum is 1.
 * - 'consume' function is called for each message received
 * - 'counter' function is called to update count of
 *    of completed produced (sent) descriptors since last update
 * - 'arg' is private to the service
 * - return value is an IPM handle ('ipmh') that can be used
 *   e.g. to de-register
 * - if the 'tag' is already in use, or if 'quota' descriptors are not available,
 *   then NULL is returned.
 */
void *gemini_ipm_register(uint8_t, unsigned int, size_t,
	void (*)(void *, const void *),
	void (*)(void *, size_t),
	void *);

/*
 * void gemini_ipm_deregister(void *ipmh);
 *
 * - tear down a service
 * - 'ipmh' is handle returned from priot call to gemini_ipm_register()
 */
void gemini_ipm_deregister(void *);

/*
 * void gemini_ipm_produce(const void *desc, unsigned size_t ndesc);
 *
 * - service produces (sends) 'ndesc' messages described by the array of
 *   descriptors 'desc'.
 * - if not all messages can be sent due to lack of descriptor queue resources,
 *   then the calling service has exceeded its quota and the system will panic.
 * - after return the descriptors at 'desc' revert to the caller
 *   caller can recycle or free as he likes.
 */
int gemini_ipm_produce(const void *, size_t);

/*
 * void gemini_ipm_copyin(void *dest, bus_addr_t ba, size_t len);
 *
 * - service copies in (receives) message 'len' bytes of data to be copied
 *   from bus address 'ba' to virtual address 'dest'
 * - this function is meant to be called from the service's registered
 *   'consume' callback function
 */
void gemini_ipm_copyin(void *, bus_addr_t, size_t);


#endif	/* _GEMINI_IPM_H_ */
