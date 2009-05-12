/* Generated from packet-show.cast by ../../util/caster.pl, do not edit. */

#include "types.h"

/*
 * (line 4) char *show_packet_tag(__ops_packet_tag_t packet_tag,
 * packet_tag_map_t *packet_tag_map) -> char *__ops_str_from_map(int
 * packet_tag, __ops_map_t *packet_tag_map)
 */
const char     *__ops_str_from_map(int packet_tag, __ops_map_t *packet_tag_map);
#define show_packet_tag(packet_tag,packet_tag_map)			\
	__ops_str_from_map(CHECKED_INSTANCE_OF(__ops_packet_tag_t, packet_tag),\
		CHECKED_INSTANCE_OF( packet_tag_map_t *, packet_tag_map))
typedef char   *show_packet_tag_t(__ops_packet_tag_t, packet_tag_map_t *);

/*
 * (line 5) char *show_sig_type(__ops_sig_type_t sig_type, sig_type_map_t
 * *sig_type_map) -> char *__ops_str_from_map(int sig_type, __ops_map_t
 * *sig_type_map)
 */
const char     *__ops_str_from_map(int sig_type, __ops_map_t * sig_type_map);
#define show_sig_type(sig_type,sig_type_map) __ops_str_from_map(CHECKED_INSTANCE_OF(__ops_sig_type_t , sig_type),CHECKED_INSTANCE_OF( sig_type_map_t *, sig_type_map))
typedef char   *show_sig_type_t(__ops_sig_type_t, sig_type_map_t *);

/*
 * (line 6) char *show_pka(__ops_pubkey_alg_t pka,
 * pubkey_alg_map_t *pka_map) -> char *__ops_str_from_map(int pka,
 * __ops_map_t *pka_map)
 */
const char     *__ops_str_from_map(int pka, __ops_map_t * pka_map);
#define show_pka(pka,pka_map) __ops_str_from_map(CHECKED_INSTANCE_OF(__ops_pubkey_alg_t , pka),CHECKED_INSTANCE_OF( pubkey_alg_map_t *, pka_map))
typedef char   *show_pka_t(__ops_pubkey_alg_t, pubkey_alg_map_t *);

/*
 * (line 7) char *show_ss_type(__ops_ss_type_t ss_type, ss_type_map_t
 * *ss_type_map) -> char *__ops_str_from_map(int ss_type, __ops_map_t
 * *ss_type_map)
 */
const char     *__ops_str_from_map(int ss_type, __ops_map_t * ss_type_map);
#define show_ss_type(ss_type,ss_type_map) __ops_str_from_map(CHECKED_INSTANCE_OF(__ops_ss_type_t , ss_type),CHECKED_INSTANCE_OF( ss_type_map_t *, ss_type_map))
typedef char   *show_ss_type_t(__ops_ss_type_t, ss_type_map_t *);

/*
 * (line 8) char *show_ss_rr_code(__ops_ss_rr_code_t ss_rr_code,
 * ss_rr_code_map_t *ss_rr_code_map) -> char *__ops_str_from_map(int
 * ss_rr_code, __ops_map_t *ss_rr_code_map)
 */
const char     *__ops_str_from_map(int ss_rr_code, __ops_map_t * ss_rr_code_map);
#define show_ss_rr_code(ss_rr_code,ss_rr_code_map) __ops_str_from_map(CHECKED_INSTANCE_OF(__ops_ss_rr_code_t , ss_rr_code),CHECKED_INSTANCE_OF( ss_rr_code_map_t *, ss_rr_code_map))
typedef char   *show_ss_rr_code_t(__ops_ss_rr_code_t, ss_rr_code_map_t *);

/*
 * (line 9) char *show_hash_alg(unsigned char hash,+__ops_map_t
 * *hash_alg_map) -> char *__ops_str_from_map(int hash,__ops_map_t
 * *hash_alg_map)
 */
const char     *__ops_str_from_map(int hash, __ops_map_t * hash_alg_map);
#define show_hash_alg(hash) __ops_str_from_map(CHECKED_INSTANCE_OF(unsigned char , hash),CHECKED_INSTANCE_OF(__ops_map_t *, hash_alg_map))
typedef char   *show_hash_alg_t(unsigned char);

/*
 * (line 10) char *show_symm_alg(unsigned char hash,+__ops_map_t
 * *symm_alg_map) -> char *__ops_str_from_map(int hash,__ops_map_t
 * *symm_alg_map)
 */
const char     *__ops_str_from_map(int hash, __ops_map_t * symm_alg_map);
#define show_symm_alg(hash) __ops_str_from_map(CHECKED_INSTANCE_OF(unsigned char , hash),CHECKED_INSTANCE_OF(__ops_map_t *, symm_alg_map))
typedef char   *show_symm_alg_t(unsigned char);
