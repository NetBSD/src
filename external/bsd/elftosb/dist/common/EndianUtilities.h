/*
 * File:	EndianUtilities.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_EndianUtilities_h_)
#define _EndianUtilities_h_

//! \name Swap macros
//! These macros always swap the data.
//@{

//! Byte swap 16-bit value.
#define _BYTESWAP16(value)                 \
        (((((uint16_t)value)<<8) & 0xFF00)   | \
         ((((uint16_t)value)>>8) & 0x00FF))

//! Byte swap 32-bit value.
#define _BYTESWAP32(value)                     \
        (((((uint32_t)value)<<24) & 0xFF000000)  | \
         ((((uint32_t)value)<< 8) & 0x00FF0000)  | \
         ((((uint32_t)value)>> 8) & 0x0000FF00)  | \
         ((((uint32_t)value)>>24) & 0x000000FF))

//! Byte swap 64-bit value.
#define _BYTESWAP64(value)                                \
		(((((uint64_t)value)<<56) & 0xFF00000000000000ULL)  | \
		 ((((uint64_t)value)<<40) & 0x00FF000000000000ULL)  | \
		 ((((uint64_t)value)<<24) & 0x0000FF0000000000ULL)  | \
		 ((((uint64_t)value)<< 8) & 0x000000FF00000000ULL)  | \
		 ((((uint64_t)value)>> 8) & 0x00000000FF000000ULL)  | \
		 ((((uint64_t)value)>>24) & 0x0000000000FF0000ULL)  | \
		 ((((uint64_t)value)>>40) & 0x000000000000FF00ULL)  | \
		 ((((uint64_t)value)>>56) & 0x00000000000000FFULL))

//@}

//! \name Inline swap functions
//@{

inline uint16_t _swap_u16(uint16_t value) { return _BYTESWAP16(value); }
inline int16_t _swap_s16(int16_t value) { return (int16_t)_BYTESWAP16((uint16_t)value); }

inline uint32_t _swap_u32(uint32_t value) { return _BYTESWAP32(value); }
inline int32_t _swap_s32(int32_t value) { return (int32_t)_BYTESWAP32((uint32_t)value); }

inline uint64_t _swap_u64(uint64_t value) { return _BYTESWAP64(value); }
inline int64_t _swap_s64(int64_t value) { return (uint64_t)_BYTESWAP64((uint64_t)value); }

//@}

#if defined(__LITTLE_ENDIAN__)

	/* little endian host */

	#define ENDIAN_BIG_TO_HOST_U16(value) (_swap_u16(value))
	#define ENDIAN_HOST_TO_BIG_U16(value) (_swap_u16(value))
	
	#define ENDIAN_BIG_TO_HOST_S16(value) (_swap_s16(value))
	#define ENDIAN_HOST_TO_BIG_S16(value) (_swap_s16(value))
	
	#define ENDIAN_BIG_TO_HOST_U32(value) (_swap_u32(value))
	#define ENDIAN_HOST_TO_BIG_U32(value) (_swap_u32(value))
	
	#define ENDIAN_BIG_TO_HOST_S32(value) (_swap_s32(value))
	#define ENDIAN_HOST_TO_BIG_S32(value) (_swap_s32(value))
	
	#define ENDIAN_BIG_TO_HOST_U64(value) (_swap_u64(value))
	#define ENDIAN_HOST_TO_BIG_U64(value) (_swap_u64(value))
	
	#define ENDIAN_BIG_TO_HOST_S64(value) (_swap_s64(value))
	#define ENDIAN_HOST_TO_BIG_S64(value) (_swap_s64(value))
	
	/* no-ops */
	
	#define ENDIAN_LITTLE_TO_HOST_U16(value) (value)
	#define ENDIAN_HOST_TO_LITTLE_U16(value) (value)

	#define ENDIAN_LITTLE_TO_HOST_S16(value) (value)
	#define ENDIAN_HOST_TO_LITTLE_S16(value) (value)
	
	#define ENDIAN_LITTLE_TO_HOST_U32(value) (value)
	#define ENDIAN_HOST_TO_LITTLE_U32(value) (value)
	
	#define ENDIAN_LITTLE_TO_HOST_S32(value) (value)
	#define ENDIAN_HOST_TO_LITTLE_S32(value) (value)
	
	#define ENDIAN_LITTLE_TO_HOST_U64(value) (value)
	#define ENDIAN_HOST_TO_LITTLE_U64(value) (value)
	
	#define ENDIAN_LITTLE_TO_HOST_S64(value) (value)
	#define ENDIAN_HOST_TO_LITTLE_S64(value) (value)
	
#elif defined(__BIG_ENDIAN__)

	/* big endian host */

	#define ENDIAN_LITTLE_TO_HOST_U16(value) (_swap_u16(value))
	#define ENDIAN_HOST_TO_LITTLE_U16(value) (_swap_u16(value))

	#define ENDIAN_LITTLE_TO_HOST_S16(value) (_swap_s16(value))
	#define ENDIAN_HOST_TO_LITTLE_S16(value) (_swap_s16(value))
	
	#define ENDIAN_LITTLE_TO_HOST_U32(value) (_swap_u32(value))
	#define ENDIAN_HOST_TO_LITTLE_U32(value) (_swap_u32(value))
	
	#define ENDIAN_LITTLE_TO_HOST_S32(value) (_swap_s32(value))
	#define ENDIAN_HOST_TO_LITTLE_S32(value) (_swap_s32(value))
	
	#define ENDIAN_LITTLE_TO_HOST_U64(value) (_swap_u64(value))
	#define ENDIAN_HOST_TO_LITTLE_U64(value) (_swap_u64(value))
	
	#define ENDIAN_LITTLE_TO_HOST_S64(value) (_swap_s64(value))
	#define ENDIAN_HOST_TO_LITTLE_S64(value) (_swap_s64(value))
	
	/* no-ops */
	
	#define ENDIAN_BIG_TO_HOST_U16(value) (value)
	#define ENDIAN_HOST_TO_BIG_U16(value) (value)
	
	#define ENDIAN_BIG_TO_HOST_S16(value) (value)
	#define ENDIAN_HOST_TO_BIG_S16(value) (value)
	
	#define ENDIAN_BIG_TO_HOST_U32(value) (value)
	#define ENDIAN_HOST_TO_BIG_U32(value) (value)
	
	#define ENDIAN_BIG_TO_HOST_S32(value) (value)
	#define ENDIAN_HOST_TO_BIG_S32(value) (value)
	
	#define ENDIAN_BIG_TO_HOST_U64(value) (value)
	#define ENDIAN_HOST_TO_BIG_U64(value) (value)
	
	#define ENDIAN_BIG_TO_HOST_S64(value) (value)
	#define ENDIAN_HOST_TO_BIG_S64(value) (value)

#endif



#endif // _EndianUtilities_h_
