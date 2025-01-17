#ifndef _URCU_UATOMIC_API_H
#define _URCU_UATOMIC_API_H

/*
 * Select second argument. Use inside macros to implement optional last macro
 * argument, such as:
 *
 * #define macro(_a, _b, _c, _optional...) \
 *     _uatomic_select_arg1(_, ##_optional, do_default_macro())
 */
#define _uatomic_select_arg1(arg0, arg1, ...) arg1

/*
 * Like _uatomic_select_arg2(), but can be used for selecting a second optional
 * argument.
 */
#define _uatomic_select_arg2(arg0, arg1, arg2, ...) arg2

#define _uatomic_default_mo(dflt, mo...)	\
	_uatomic_select_arg1(_, ##mo, dflt)

#define _uatomic_default_mo2(dflt, mo...)	\
	_uatomic_select_arg2(_, ##mo, dflt, dflt)

#define uatomic_load(addr, mo...)		\
	uatomic_load_mo(addr, _uatomic_default_mo(CMM_RELAXED, ##mo))

#define uatomic_read(addr, mo...)					\
	uatomic_load_mo(addr, _uatomic_default_mo(CMM_RELAXED, ##mo))

#define uatomic_store(addr, value, mo...)				\
	uatomic_store_mo(addr, value, _uatomic_default_mo(CMM_RELAXED, ##mo))

#define uatomic_set(addr, value, mo...)					\
	uatomic_store_mo(addr, value, _uatomic_default_mo(CMM_RELAXED, ##mo))

#define uatomic_add_return(addr, v, mo...)				\
	uatomic_add_return_mo(addr, v, _uatomic_default_mo(CMM_SEQ_CST_FENCE, ##mo))

#define uatomic_sub_return(addr, v, mo...)				\
	uatomic_sub_return_mo(addr, v, _uatomic_default_mo(CMM_SEQ_CST_FENCE, ##mo))

#define uatomic_and(addr, mask, mo...)					\
	uatomic_and_mo(addr, mask, _uatomic_default_mo(CMM_SEQ_CST, ##mo))

#define uatomic_or(addr, mask, mo...)						\
	uatomic_or_mo(addr, mask, _uatomic_default_mo(CMM_RELAXED, ##mo))

#define uatomic_add(addr, v, mo...)						\
	uatomic_add_mo(addr, v, _uatomic_default_mo(CMM_RELAXED, ##mo))

#define uatomic_sub(addr, v, mo...)						\
	uatomic_sub_mo(addr, v, _uatomic_default_mo(CMM_RELAXED, ##mo))

#define uatomic_inc(addr, mo...)						\
	uatomic_inc_mo(addr, _uatomic_default_mo(CMM_RELAXED, ##mo))

#define uatomic_dec(addr, mo...)						\
	uatomic_dec_mo(addr, _uatomic_default_mo(CMM_RELAXED, ##mo))

#define uatomic_xchg(addr, value, mo...)				\
	uatomic_xchg_mo(addr, value,					\
			_uatomic_default_mo(CMM_SEQ_CST_FENCE, ##mo))

#define uatomic_cmpxchg(addr, value, _new, mo...)			\
	uatomic_cmpxchg_mo(addr, value,	_new,				\
			_uatomic_default_mo(CMM_SEQ_CST_FENCE, ##mo),	\
			_uatomic_default_mo2(CMM_RELAXED, ##mo))


#endif	/* _URUC_UATOMIC_API_H */
