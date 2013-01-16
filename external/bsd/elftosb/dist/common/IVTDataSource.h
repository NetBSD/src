/*
 * File:	DataSource.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 *
 * Freescale Semiconductor, Inc.
 * Proprietary & Confidential
 *
 * This source code and the algorithms implemented therein constitute
 * confidential information and may comprise trade secrets of Freescale Semiconductor, Inc.
 * or its associates, and any use thereof is subject to the terms and
 * conditions of the Confidential Disclosure Agreement pursual to which this
 * source code was originally received.
 */
#if !defined(_IVTDataSource_h_)
#define _IVTDataSource_h_

#include "DataSource.h"

/** Header field components
 * @ingroup hdr
 */
typedef struct hab_hdr {
    uint8_t tag;              /**< Tag field */
    uint8_t len[2];           /**< Length field in bytes (big-endian) */
    uint8_t par;              /**< Parameters field */
} hab_hdr_t;

/** Image entry function prototype
 *  @ingroup rvt
 *
 * This typedef serves as the return type for hab_rvt.authenticate_image().  It
 * specifies a void-void function pointer, but can be cast to another function
 * pointer type if required.
 */
typedef void (*hab_image_entry_f)(void);

/** @ref ivt structure
 * @ingroup ivt
 *
 * @par Format
 *
 * An @ref ivt consists of a @ref hdr followed by a list of addresses as
 * described further below.
 *
 * @warning The @a entry address may not be NULL.
 *
 * @warning On an IC not configured as #HAB_CFG_CLOSED, the
 * @a csf address may be NULL.  If it is not NULL, the @ref csf will be
 * processed, but any failures should be non-fatal.
 *
 * @warning On an IC configured as #HAB_CFG_CLOSED, the @a
 * csf address may not be NULL, and @ref csf failures are typically fatal.
 *
 * @remark The Boot Data located using the @a boot_data field is interpreted
 * by the HAB caller in a boot-mode specific manner.  This may be used by the
 * boot ROM as to determine the load address and boot device configuration for
 * images loaded from block devices (see @ref ref_rug for details).
 *
 * @remark All addresses given in the IVT, including the Boot Data (if
 * present) are those for the final load location. 
 *
 * @anchor ila
 *
 * @par Initial load addresses
 *
 * The @a self field is used to calculate addresses in boot modes where an
 * initial portion of the image is loaded to an initial location.  In such
 * cases, the IVT, Boot Data (if present) and DCD (if present) are used in
 * configuring the IC and loading the full image to its final location.  Only
 * the IVT, Boot Data (if present) and DCD (if present) are required to be
 * within the initial image portion.
 *
 * The method for calculating an initial load address for the DCD is
 * illustrated in the following C fragment.  Similar calculations apply to
 * other fields.
 *
@verbatim
        hab_ivt_t* ivt_initial = <initial IVT load address>;
        const void* dcd_initial = ivt_initial->dcd;
        if (ivt_initial->dcd != NULL)
            dcd_initial = (const uint8_t*)ivt_initial 
                          + (ivt_initial->dcd - ivt_initial->self)
@endverbatim

 * \note The void* types in this structure have been changed to uint32_t so
 *      that this code will work correctly when compiled on a 64-bit host.
 *      Otherwise the structure would come out incorrect.
 */
struct hab_ivt {
    /** @ref hdr with tag #HAB_TAG_IVT, length and HAB version fields
     *  (see @ref data)
     */
    hab_hdr_t hdr;
    /** Absolute address of the first instruction to execute from the
     *  image
     */
    /*hab_image_entry_f*/ uint32_t entry;
    /** Reserved in this version of HAB: should be NULL. */
    /*const void*/ uint32_t reserved1;
    /** Absolute address of the image DCD: may be NULL. */
    /*const void*/ uint32_t dcd;
    /** Absolute address of the Boot Data: may be NULL, but not interpreted
     *  any further by HAB
     */
    /*const void*/ uint32_t boot_data;
    /** Absolute address of the IVT.*/
    /*const void*/ uint32_t self;
    /** Absolute address of the image CSF.*/
    /*const void*/ uint32_t csf;
    /** Reserved in this version of HAB: should be zero. */
    uint32_t reserved2;
};

/** @ref ivt type
 * @ingroup ivt
 */
typedef struct hab_ivt hab_ivt_t;

/*
 *    Helper macros
 */
#define HAB_CMD_UNS     0xff

#define DEFAULT_IMG_KEY_IDX     2

#define GEN_MASK(width)                         \
    ((1UL << (width)) - 1)

#define GEN_FIELD(f, width, shift)              \
    (((f) & GEN_MASK(width)) << (shift))

#define PACK_UINT32(a, b, c, d)                 \
    ( (((a) & 0xFF) << 24)                      \
      |(((b) & 0xFF) << 16)                     \
      |(((c) & 0xFF) << 8)                      \
      |(((d) & 0xFF)) )

#define EXPAND_UINT32(w)                                                \
    (uint8_t)((w)>>24), (uint8_t)((w)>>16), (uint8_t)((w)>>8), (uint8_t)(w)

#define HDR(tag, bytes, par)                                            \
    (uint8_t)(tag), (uint8_t)((bytes)>>8), (uint8_t)(bytes), (uint8_t)(par)

#define HAB_VER(maj, min)                                       \
    (GEN_FIELD((maj), HAB_VER_MAJ_WIDTH, HAB_VER_MAJ_SHIFT)     \
     | GEN_FIELD((min), HAB_VER_MIN_WIDTH, HAB_VER_MIN_SHIFT))

/*
 *    CSF header
 */

#define CSF_HDR(bytes, HABVER)                  \
    HDR(HAB_TAG_CSF, (bytes), HABVER)
    
    
/*
 *    DCD  header
 */

#define DCD_HDR(bytes, HABVER)                  \
    HDR(HAB_TAG_DCD, (bytes), HABVER)

/*
 *   IVT  header (goes in the struct's hab_hdr_t field, not a byte array)
 */
#define IVT_HDR(bytes, HABVER)                  \
    {HAB_TAG_IVT, {(uint8_t)((bytes)>>8), (uint8_t)(bytes)}, HABVER}

/** @name External data structure tags
 * @anchor dat_tag
 *
 * Tag values 0x00 .. 0xef are reserved for HAB.  Values 0xf0 .. 0xff
 * are available for custom use.
 */
/*@{*/
#define HAB_TAG_IVT  0xd1       /**< Image Vector Table */
#define HAB_TAG_DCD  0xd2       /**< Device Configuration Data */
#define HAB_TAG_CSF  0xd4       /**< Command Sequence File */
#define HAB_TAG_CRT  0xd7       /**< Certificate */
#define HAB_TAG_SIG  0xd8       /**< Signature */
#define HAB_TAG_EVT  0xdb       /**< Event */
#define HAB_TAG_RVT  0xdd       /**< ROM Vector Table */
/* Values b0 ... cf reserved for CSF commands.  Values e0 ... ef reserved for
 * key types.
 *
 * Available values: 03, 05, 06, 09, 0a, 0c, 0f, 11, 12, 14, 17, 18, 1b, 1d,
 * 1e, 21, 22, 24, 27, 28, 2b, 2d, 2e, 30, 33, 35, 36, 39, 3a, 3c, 3f, 41, 42,
 * 44, 47, 48, 4b, 4d, 4e, 50, 53, 55, 56, 59, 5a, 5c, 5f, 60, 63, 65, 66, 69,
 * 6a, 6c, 6f, 71, 72, 74, 77, 78, 7b, 7d, 7e, 81, 82, 84, 87, 88, 8b, 8d, 8e,
 * 90, 93, 95, 96, 99, 9a, 9c, 9f, a0, a3, a5, a6, a9, aa, ac, af, b1, b2, b4,
 * b7, b8, bb, bd, be
 *
 * Custom values: f0, f3, f5, f6, f9, fa, fc, ff
 */
/*@}*/

/** @name HAB version */
/*@{*/
#define HAB_MAJOR_VERSION  4    /**< Major version of this HAB release */
#define HAB_MINOR_VERSION  0    /**< Minor version of this HAB release */
#define HAB_VER_MAJ_WIDTH 4     /**< Major version field width  */
#define HAB_VER_MAJ_SHIFT 4     /**< Major version field offset  */
#define HAB_VER_MIN_WIDTH 4     /**< Minor version field width  */
#define HAB_VER_MIN_SHIFT 0     /**< Minor version field offset  */
/** Full version of this HAB release @hideinitializer */
#define HAB_VERSION HAB_VER(HAB_MAJOR_VERSION, HAB_MINOR_VERSION) 
/** Base version for this HAB release @hideinitializer */
#define HAB_BASE_VERSION HAB_VER(HAB_MAJOR_VERSION, 0) 

/*@}*/

namespace elftosb {

/*!
 * \brief Data source for an IVT structure used by HAB4.
 *
 * This data source represents an IVT structure used by HAB4. Fields of the IVT can be set
 * by name, making it easy to interface with a parser. And it has some intelligence regarding
 * the IVT's self pointer. Before the data is copied out by the getData() method, the self field
 * will be filled in automatically if it has not already been set and there is an associated
 * data target object. This lets the IVT pick up its own address from the location where it is
 * being loaded. Alternatively, if the self pointer is filled in explicitly, then the data
 * source will have a natural location equal to the self pointer.
 *
 * This data source acts as its own segment.
 */
class IVTDataSource : public DataSource, public DataSource::Segment
{
public:
    //! \brief Default constructor.
    IVTDataSource();
    
	//! \brief There is only one segment.
	virtual unsigned getSegmentCount() { return 1; }
	
	//! \brief Returns this object, as it is its own segment.
	virtual DataSource::Segment * getSegmentAt(unsigned index) { return this; }

    //! \name Segment methods
    //@{
    
    //! \brief Copy out some or all of the IVT structure.
    //!
    virtual unsigned getData(unsigned offset, unsigned maxBytes, uint8_t * buffer);
    
    //! \brief Gets the length of the segment's data.
    virtual unsigned getLength();
    
    //! \brief Returns whether the segment has an associated address.
    virtual bool hasNaturalLocation();
    
    //! \brief Returns the address associated with the segment.
    virtual uint32_t getBaseAddress();
    
    //@}
    
    //! \name IVT access
    //@{
    
    //! \brief Set one of the IVT's fields by providing its name.
    //!
    //! Acceptable field names are:
    //! - entry
    //! - dcd
    //! - boot_data
    //! - self
    //! - csf
    //!
    //! As long as the \a name parameter specifies one of these fields, the return value
    //! will be true. If \a name contains any other value, then false will be returned and
    //! the IVT left unmodified.
    //!
    //! Once the \a self field has been set to any value, the data source will have a
    //! natural location. This works even if the \a self address is 0.
    //!
    //! \param name The name of the field to set. Field names are case sensitive, just like in
    //!     the C language.
    //! \param value The value to which the field will be set.
    //! \retval true The field was set successfully.
    //! \retval false There is no field with the provided name.
    bool setFieldByName(const std::string & name, uint32_t value);
    
    //! \brief Returns a reference to the IVT structure.
    hab_ivt_t & getIVT() { return m_ivt; }
    
    //@}

protected:
    hab_ivt_t m_ivt;  //!< The IVT structure.
    bool m_isSelfSet; //!< True if the IVT self pointer was explicitly set.
};

} // elftosb

#endif // _IVTDataSource_h_
