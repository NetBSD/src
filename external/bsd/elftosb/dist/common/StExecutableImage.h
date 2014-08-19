/*
 * File:	StExecutableImage.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_StExecutableImage_h_)
#define _StExecutableImage_h_

#include "stdafx.h"
#include <list>

/*!
 * \brief Used to build a representation of memory regions.
 *
 * An intermediate representation of the memory regions and segments loaded
 * from an executable file. Also used to find contiguous segments that are
 * specified separately in the source file.
 *
 * When regions are added, an attempt is made to coalesce contiguous regions.
 * In order for this to succeed, the touching regions must be of the same
 * type and have the same permissions. Regions are also kept sorted by their
 * address range as they are added.
 *
 * \todo Implement alignment support.
 */
class StExecutableImage
{
public:
	//! Possible types of memory regions.
	typedef enum {
		TEXT_REGION,	//!< A region containing data or instructions.
		FILL_REGION		//!< Region to be initialized with zero bytes.
	} MemoryRegionType;
	
	//! Memory region flag constants.
	enum {
		REGION_READ_FLAG = 1,	//!< Region is readable.
		REGION_WRITE_FLAG = 2,	//!< Region is writable.
		REGION_EXEC_FLAG = 4,	//!< Region may contain executable code.
		
		REGION_RW_FLAG = REGION_READ_FLAG | REGION_WRITE_FLAG,	//!< Region is read-write.
		
		//! Mask to access only permissions flags for a region.
		REGION_PERM_FLAG_MASK = 0x7
	};
	
	/*!
	 * Representation of a contiguous region of memory.
     *
     * \todo Add comparison operators so we can use the STL sort algorithm.
	 */
	struct MemoryRegion
	{
		MemoryRegionType m_type;	//!< Memory region type.
		uint32_t m_address;	//!< The 32-bit start address of this region.
		uint32_t m_length;	//!< Number of bytes in this region.
		uint8_t * m_data;	//!< Pointer to data. Will be NULL for FILL_REGION type.
		unsigned m_flags;	//!< Flags for the region.
        
        //! \brief Calculates the address of the last byte occupied by this region.
        inline uint32_t endAddress() const { return m_address + m_length - 1; }
        
        //! \brief Equality operator.
        bool operator == (const MemoryRegion & other) const;
	};
	
	//! A list of #StExecutableImage::MemoryRegion objects.
	typedef std::list<MemoryRegion> MemoryRegionList;
    
    //! The iterator type used to access #StExecutableImage::MemoryRegion objects. This type
    //! is used by the methods #getRegionBegin() and #getRegionEnd().
	typedef MemoryRegionList::const_iterator const_iterator;
	
    //! The possible actions for regions matching an address filter range.
    typedef enum {
        ADDR_FILTER_NONE,       //!< Do nothing.
        ADDR_FILTER_ERROR,      //!< Raise an error exception.
        ADDR_FILTER_WARNING,    //!< Raise a warning exception.
        ADDR_FILTER_CROP        //!< Don't include the matching address range in the executable image.
    } AddressFilterAction;
    
    /*!
     * An address filter consists of a single address range and an action. If a
     * memory region overlaps the filter's range then the action will be performed.
     * The possible filter actions are defined by the #AddressFilterAction enumeration.
     */
    struct AddressFilter
    {
        AddressFilterAction m_action;   //!< Action to be performed when the filter is matched.
        uint32_t m_fromAddress; //!< Start address of the filter. Should be lower than or equal to #m_toAddress.
        uint32_t m_toAddress;   //!< End address of the filter. Should be higher than or equal to #m_fromAddress.
        unsigned m_priority;     //!< Priority for this filter. Zero is the lowest priority.
        
        //! \brief Constructor.
        AddressFilter(AddressFilterAction action, uint32_t from, uint32_t to, unsigned priority=0)
        :   m_action(action), m_fromAddress(from), m_toAddress(to), m_priority(priority)
        {
        }
        
        //! \brief Test routine.
        bool matchesMemoryRegion(const MemoryRegion & region) const;
        
        //! \brief Compares two address filter objects.
        int compare(const AddressFilter & other) const;
        
        //! \name Comparison operators
        //@{
        inline bool operator < (const AddressFilter & other) const { return compare(other) == -1; }
        inline bool operator > (const AddressFilter & other) const { return compare(other) == 1; }
        inline bool operator == (const AddressFilter & other) const { return compare(other) == 0; }
        inline bool operator <= (const AddressFilter & other) const { return compare(other) != 1; }
        inline bool operator >= (const AddressFilter & other) const { return compare(other) != -1; }
        //@}
    };
    
    //! List of #StExecutableImage::AddressFilter objects.
    typedef std::list<AddressFilter> AddressFilterList;
    
    //! The exception class raised for the #ADDR_FILTER_ERROR and #ADDR_FILTER_WARNING
    //! filter actions.
    class address_filter_exception
    {
    public:
        //! \brief Constructor.
        //!
        //! A local copy of \a matchingFilter is made, in case the image and/or filter
        //! are on the stack and would be disposed of when the exception is raised.
        address_filter_exception(bool isError, std::string & imageName, const AddressFilter & matchingFilter)
        : m_isError(isError), m_imageName(imageName), m_filter(matchingFilter)
        {
        }
        
        //! \brief Returns true if the exception is an error. Otherwise the exception
        //!     is for a warning.
        inline bool isError() const { return m_isError; }
        
        //! \brief
        inline std::string getImageName() const { return m_imageName; }
        
        //! \brief
        inline const AddressFilter & getMatchingFilter() const { return m_filter; }
    
    protected:
        bool m_isError;
        std::string m_imageName;
        AddressFilter m_filter;
    };
    
public:
	//! \brief Constructor.
	StExecutableImage(int inAlignment=256);
	
	//! \brief Copy constructor.
	StExecutableImage(const StExecutableImage & inOther);
	
	//! \brief Destructor.
	virtual ~StExecutableImage();
	
	//! \name Image name
	//! Methods for getting and setting the image name.
	//@{
	//! \brief Sets the image's name to \a inName.
	virtual void setName(const std::string & inName);
	
	//! \brief Returns a copy of the image's name.
	virtual std::string getName() const;
	//@}
	
	//! \name Regions
	//! Methods to add and access memory regions.
	//@{
	//! \brief Add a region to be filled with zeroes.
	virtual void addFillRegion(uint32_t inAddress, unsigned inLength);
	
	//! \brief Add a region containing data to be loaded.
	virtual void addTextRegion(uint32_t inAddress, const uint8_t * inData, unsigned inLength);
	
	//! \brief Returns the total number of regions.
	//!
	//! Note that this count may not be the same as the number of calls to
	//! addFillRegion() and addTextRegion() due to region coalescing.
	inline unsigned getRegionCount() const { return static_cast<unsigned>(m_image.size()); }
	
	//! \brief Returns a reference to the region specified by \a inIndex.
	const MemoryRegion & getRegionAtIndex(unsigned inIndex) const;
	
    //! \brief Return an iterator to the first region.
	inline const_iterator getRegionBegin() const { return m_image.begin(); }
    
    //! \brief Return an iterator to the next-after-last region.
	inline const_iterator getRegionEnd() const { return m_image.end(); }
	//@}
	
	//! \name Entry point
	//@{
	//! \brief Sets the entry point address.
	inline void setEntryPoint(uint32_t inEntryAddress) { m_entry = inEntryAddress; m_hasEntry = true; }
	
	//! \brief Returns true if an entry point has been set.
	inline bool hasEntryPoint() const { return m_hasEntry; }
	
	//! \brief Returns the entry point address.
	inline uint32_t getEntryPoint() const { return hasEntryPoint() ? m_entry : 0; }
	//@}
    
    //! \name Address filter
    //@{
    //! \brief Add a new address filter.
    virtual void addAddressFilter(const AddressFilter & filter);
    
    //! \brief Add multiple address filters at once.
    //!
    //! The template argument \a _T must be an iterator or const iterator that
    //! dereferences to an StExecutableImage::AddressFilter reference. All filters
    //! from \a from to \a to will be added to the address filter list.
    template<typename _T> void addAddressFilters(_T from, _T to)
    {
        _T it = from;
        for (; it != to; ++it)
        {
            addAddressFilter(*it);
        }
    }
    
    //! \brief Remove all active filters.
    virtual void clearAddressFilters();
    
    //! \brief Process all active filters and perform associated actions.
    virtual void applyAddressFilters();
    //@}
	
protected:
	std::string m_name;	//!< The name of the image (can be a file name, for instance).
	int m_alignment;	//!< The required address alignment for each memory region.
	bool m_hasEntry;	//!< True if an entry point has been set.
	uint32_t m_entry;	//!< Entry point address.
	MemoryRegionList m_image;	//!< The memory regions.
    AddressFilterList m_filters;    //!< List of active address filters.
    
    //! \brief Deletes the portion \a region that overlaps \a filter.
    void cropRegionToFilter(MemoryRegion & region, const AddressFilter & filter);
	
	//! \brief Inserts the region in sorted order or merges with one already in the image.
	void insertOrMergeRegion(MemoryRegion & inRegion);
	
	//! \brief Merges two memory regions into one.
	void mergeRegions(MemoryRegion & inOldRegion, MemoryRegion & inNewRegion);
};

#endif // _StExecutableImage_h_
