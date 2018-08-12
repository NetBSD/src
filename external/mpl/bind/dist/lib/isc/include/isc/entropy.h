/*	$NetBSD: entropy.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_ENTROPY_H
#define ISC_ENTROPY_H 1

/*****
 ***** Module Info
 *****/

/*! \file isc/entropy.h
 * \brief The entropy API
 *
 * \li MP:
 *	The entropy object is locked internally.  All callbacks into
 *	application-provided functions (for setup, gathering, and
 *	shutdown of sources) are guaranteed to be called with the
 *	entropy API lock held.  This means these functions are
 *	not permitted to call back into the entropy API.
 *
 * \li Reliability:
 *	No anticipated impact.
 *
 * \li Resources:
 *	A buffer, used as an entropy pool.
 *
 * \li Security:
 *	While this code is believed to implement good entropy gathering
 *	and distribution, it has not been reviewed by a cryptographic
 *	expert.
 *	Since the added entropy is only as good as the sources used,
 *	this module could hand out bad data and never know it.
 *
 * \li Standards:
 *	None.
 */

/***
 *** Imports
 ***/

#include <stdio.h>

#include <isc/lang.h>
#include <isc/types.h>

/*@{*/
/*% Entropy callback function. */
typedef isc_result_t (*isc_entropystart_t)(isc_entropysource_t *source,
					   void *arg, isc_boolean_t blocking);
typedef isc_result_t (*isc_entropyget_t)(isc_entropysource_t *source,
					 void *arg, isc_boolean_t blocking);
typedef void (*isc_entropystop_t)(isc_entropysource_t *source, void *arg);
/*@}*/

/***
 *** Flags.
 ***/

/*!
 * \brief
 *	Extract only "good" data; return failure if there is not enough
 *	data available and there are no sources which we can poll to get
 *	data, or those sources are empty.
 *
 *
 */
#define ISC_ENTROPY_GOODONLY	0x00000001U
/*!
 * \brief
 *	Extract as much good data as possible, but if there isn't enough
 *	at hand, return what is available.  This flag only makes sense
 *	when used with _GOODONLY.
 */
#define ISC_ENTROPY_PARTIAL	0x00000002U
/*!
 * \brief
 *	Block the task until data is available.  This is contrary to the
 *	ISC task system, where tasks should never block.  However, if
 *	this is a special purpose application where blocking a task is
 *	acceptable (say, an offline zone signer) this flag may be set.
 *	This flag only makes sense when used with _GOODONLY, and will
 *	block regardless of the setting for _PARTIAL.
 */
#define ISC_ENTROPY_BLOCKING	0x00000004U

/*!
 * \brief
 *	Estimate the amount of entropy contained in the sample pool.
 *	If this is not set, the source will be gathered and periodically
 *	mixed into the entropy pool, but no increment in contained entropy
 *	will be assumed.  This flag only makes sense on sample sources.
 */
#define ISC_ENTROPYSOURCE_ESTIMATE	0x00000001U

/*
 * For use with isc_entropy_usebestsource().
 */
/*!
 * \brief
 *	Use the keyboard as the only entropy source.
 */
#define ISC_ENTROPY_KEYBOARDYES		1
/*!
 * \brief
 *	Never use the keyboard as an entropy source.
 */
#define ISC_ENTROPY_KEYBOARDNO		2
/*!
 * \brief
 *	Use the keyboard as an entropy source only if opening the
 *	random device fails.
 */
#define ISC_ENTROPY_KEYBOARDMAYBE	3

ISC_LANG_BEGINDECLS

/***
 *** Functions
 ***/

isc_result_t
isc_entropy_create(isc_mem_t *mctx, isc_entropy_t **entp);
/*!<
 * \brief Create a new entropy object.
 */

void
isc_entropy_attach(isc_entropy_t *ent, isc_entropy_t **entp);
/*!<
 * Attaches to an entropy object.
 */

void
isc_entropy_detach(isc_entropy_t **entp);
/*!<
 * \brief Detaches from an entropy object.
 */

isc_result_t
isc_entropy_createfilesource(isc_entropy_t *ent, const char *fname);
/*!<
 * \brief Create a new entropy source from a file.
 *
 * The file is assumed to contain good randomness, and will be mixed directly
 * into the pool with every byte adding 8 bits of entropy.
 *
 * The file will be put into non-blocking mode, so it may be a device file,
 * such as /dev/random.  /dev/urandom should not be used here if it can
 * be avoided, since it will always provide data even if it isn't good.
 * We will make as much pseudorandom data as we need internally if our
 * caller asks for it.
 *
 * If we hit end-of-file, we will stop reading from this source.  Callers
 * who require strong random data will get failure when our pool drains.
 * The file will never be opened/read again once EOF is reached.
 */

void
isc_entropy_destroysource(isc_entropysource_t **sourcep);
/*!<
 * \brief Removes an entropy source from the entropy system.
 */

isc_result_t
isc_entropy_createsamplesource(isc_entropy_t *ent,
			       isc_entropysource_t **sourcep);
/*!<
 * \brief Create an entropy source that consists of samples.  Each sample is
 * added to the source via isc_entropy_addsamples(), below.
 */

isc_result_t
isc_entropy_createcallbacksource(isc_entropy_t *ent,
				 isc_entropystart_t start,
				 isc_entropyget_t get,
				 isc_entropystop_t stop,
				 void *arg,
				 isc_entropysource_t **sourcep);
/*!<
 * \brief Create an entropy source that is polled via a callback.
 *
 * This would be used when keyboard input is used, or a GUI input method.
 * It can also be used to hook in any external entropy source.
 *
 * Samples are added via isc_entropy_addcallbacksample(), below.
 * _addcallbacksample() is the only function which may be called from
 * within an entropy API callback function.
 */

void
isc_entropy_stopcallbacksources(isc_entropy_t *ent);
/*!<
 * \brief Call the stop functions for callback sources that have had their
 * start functions called.
 */

/*@{*/
isc_result_t
isc_entropy_addcallbacksample(isc_entropysource_t *source, isc_uint32_t sample,
			      isc_uint32_t extra);
isc_result_t
isc_entropy_addsample(isc_entropysource_t *source, isc_uint32_t sample,
		      isc_uint32_t extra);
/*!<
 * \brief Add a sample to the sample source.
 *
 * The sample MUST be a timestamp
 * that increases over time, with the exception of wrap-around for
 * extremely high resolution timers which will quickly wrap-around
 * a 32-bit integer.
 *
 * The "extra" parameter is used only to add a bit more unpredictable
 * data.  It is not used other than included in the hash of samples.
 *
 * When in an entropy API callback function, _addcallbacksource() must be
 * used.  At all other times, _addsample() must be used.
 */
/*@}*/

isc_result_t
isc_entropy_getdata(isc_entropy_t *ent, void *data, unsigned int length,
		    unsigned int *returned, unsigned int flags);
/*!<
 * \brief Get random data from entropy pool 'ent'.
 *
 * If a hook has been set up using isc_entropy_sethook() and
 * isc_entropy_usehook(), then the hook function will be called to get
 * random data.
 *
 * Otherwise, randomness is extracted from the entropy pool set up in BIND.
 * This may cause the pool to be loaded from various sources. Ths is done
 * by stirring the pool and returning a part of hash as randomness.
 * (Note that no secrets are given away here since parts of the hash are
 * XORed together before returning.)
 *
 * 'flags' may contain ISC_ENTROPY_GOODONLY, ISC_ENTROPY_PARTIAL, or
 * ISC_ENTROPY_BLOCKING. These will be honored if the hook function is
 * not in use. If it is, the flags will be passed to the hook function
 * but it may ignore them.
 *
 * Up to 'length' bytes of randomness are retrieved and copied into 'data'.
 * (If 'returned' is not NULL, and the number of bytes copied is less than
 * 'length' - which may happen if ISC_ENTROPY_PARTIAL was used - then the
 * number of bytes copied will be stored in *returned.)
 *
 * Returns:
 * \li	ISC_R_SUCCESS on success
 * \li	ISC_R_NOENTROPY if entropy pool is empty
 * \li	other error codes are possible when a hook is in use
 */

void
isc_entropy_putdata(isc_entropy_t *ent, void *data, unsigned int length,
		    isc_uint32_t entropy);
/*!<
 * \brief Add "length" bytes in "data" to the entropy pool, incrementing the
 * pool's entropy count by "entropy."
 *
 * These bytes will prime the pseudorandom portion even if no entropy is
 * actually added.
 */

void
isc_entropy_stats(isc_entropy_t *ent, FILE *out);
/*!<
 * \brief Dump some (trivial) stats to the stdio stream "out".
 */

unsigned int
isc_entropy_status(isc_entropy_t *end);
/*
 * Returns the number of bits the pool currently contains.  This is just
 * an estimate.
 */

isc_result_t
isc_entropy_usebestsource(isc_entropy_t *ectx, isc_entropysource_t **source,
			  const char *randomfile, int use_keyboard);
/*!<
 * \brief Use whatever source of entropy is best.
 *
 * Notes:
 *\li	If "randomfile" is not NULL, open it with
 *	isc_entropy_createfilesource().
 *
 *\li	If "randomfile" is NULL and the system's random device was detected
 *	when the program was configured and built, open that device with
 *	isc_entropy_createfilesource().
 *
 *\li	If "use_keyboard" is #ISC_ENTROPY_KEYBOARDYES, then always open
 *	the keyboard as an entropy source (possibly in addition to
 *	"randomfile" or the random device).
 *
 *\li	If "use_keyboard" is #ISC_ENTROPY_KEYBOARDMAYBE, open the keyboard only
 *	if opening the random file/device fails.  A message will be
 *	printed describing the need for keyboard input.
 *
 *\li	If "use_keyboard" is #ISC_ENTROPY_KEYBOARDNO, the keyboard will
 *	never be opened.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS if at least one source of entropy could be started.
 *
 *\li	#ISC_R_NOENTROPY if use_keyboard is #ISC_ENTROPY_KEYBOARDNO and
 *	there is no random device pathname compiled into the program.
 *
 *\li	A return code from isc_entropy_createfilesource() or
 *	isc_entropy_createcallbacksource().
 */

void
isc_entropy_usehook(isc_entropy_t *ectx, isc_boolean_t onoff);
/*!<
 * \brief Configure entropy context 'ectx' to use the hook function
 *
 * Sets the entropy context to call the hook function for random number
 * generation, if such a function has been configured via
 * isc_entropy_sethook(), whenever isc_entropy_getdata() is called.
 */

void
isc_entropy_sethook(isc_entropy_getdata_t myhook);
/*!<
 * \brief Set the hook function.
 *
 * The hook function is a global value: only one hook function
 * can be set in the system. Individual entropy contexts may be
 * configured to use it, or not, by calling isc_entropy_usehook().
 */

ISC_LANG_ENDDECLS

#endif /* ISC_ENTROPY_H */
