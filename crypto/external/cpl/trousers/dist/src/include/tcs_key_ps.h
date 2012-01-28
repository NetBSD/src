
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */

#ifndef _TCS_KEY_PS_H_
#define _TCS_KEY_PS_H_

TSS_RESULT ps_init_disk_cache();
void       ps_close_disk_cache();
TSS_BOOL   ps_is_key_registered(TCPA_STORE_PUBKEY *);
TSS_RESULT getParentUUIDByUUID(TSS_UUID *, TSS_UUID *);
TSS_RESULT isUUIDRegistered(TSS_UUID *, TSS_BOOL *);
void       disk_cache_shift(struct key_disk_cache *);
TSS_RESULT ps_remove_key(TSS_UUID *);
TSS_RESULT clean_disk_cache(int);
TSS_RESULT ps_get_key_by_uuid(TSS_UUID *, BYTE *, UINT16 *);
TSS_RESULT ps_get_key_by_cache_entry(struct key_disk_cache *, BYTE *, UINT16 *);
TSS_RESULT ps_is_pub_registered(TCPA_STORE_PUBKEY *);
TSS_RESULT ps_get_uuid_by_pub(TCPA_STORE_PUBKEY *, TSS_UUID **);
TSS_RESULT ps_get_key_by_pub(TCPA_STORE_PUBKEY *, UINT32 *, BYTE **);
TSS_RESULT ps_write_key(TSS_UUID *, TSS_UUID *, BYTE *, UINT32, BYTE *, UINT32);

#endif
