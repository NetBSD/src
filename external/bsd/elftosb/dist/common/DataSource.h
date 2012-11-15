/*
 * File:	DataSource.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_DataSource_h_)
#define _DataSource_h_

#include <vector>
#include "Value.h"
#include "smart_ptr.h"
#include "StExecutableImage.h"

namespace elftosb
{

// Forward declaration
class DataTarget;

/*!
 * \brief Abstract base class for data sources.
 *
 * Data sources represent any sort of data that can be placed or loaded
 * into a target region. Sources may be a single blob of data read from
 * a file or may consist of many segments.
 *
 * The three most important features of data sources are:
 * - Sources may be multi-segmented.
 * - Sources and/or segments can have a "natural" or default target location.
 * - The target for a source may be taken into consideration when the source
 *		describes itself.
 */
class DataSource
{
public:
	/*!
	 * \brief Discrete, contiguous part of the source's data.
	 *
	 * This class is purely abstract and subclasses of DataSource are expected
	 * to subclass it to implement a segment particular to their needs.
	 */
	class Segment
	{
	public:
		//! \brief Default constructor.
		Segment(DataSource & source) : m_source(source) {}
		
		//! \brief Destructor.
		virtual ~Segment() {}
		
		//! \brief Gets all or a portion of the segment's data.
		//!
		//! The data is copied into \a buffer. Up to \a maxBytes bytes may be
		//! copied, so \a buffer must be at least that large.
		//!
		//! \param offset Index of the first byte to start copying from.
		//! \param maxBytes The maximum number of bytes that can be returned. \a buffer
		//!		must be at least this large.
		//! \param buffer Pointer to buffer where the data is copied.
		//! \return The number of bytes copied into \a buffer.
		virtual unsigned getData(unsigned offset, unsigned maxBytes, uint8_t * buffer)=0;
		
		//! \brief Gets the length of the segment's data.
		virtual unsigned getLength()=0;
		
		//! \brief Returns whether the segment has an associated address.
		virtual bool hasNaturalLocation()=0;
		
		//! \brief Returns the address associated with the segment.
		virtual uint32_t getBaseAddress() { return 0; }
	
	protected:
		DataSource & m_source;	//!< The data source to which this segment belongs.
	};
	
	/*!
	 * \brief This is a special type of segment containing a repeating pattern.
	 *
	 * By default the segment doesn't have a specific length or data. The length depends
	 * on the target's address range. And the data is just the pattern, repeated
	 * many times. In addition, pattern segments do not have a natural location.
	 *
	 * Calling code should look for instances of PatternSegment and handle them
	 * as special cases that can be optimized.
	 */
	class PatternSegment : public Segment
	{
	public:
		//! \brief Default constructor.
		PatternSegment(DataSource & source);
		
		//! \brief Constructor taking a fill pattern.
		PatternSegment(DataSource & source, const SizedIntegerValue & pattern);
		
		//! \brief Constructor taking a byte fill pattern.
		PatternSegment(DataSource & source, uint8_t pattern);
		
		//! \brief Constructor taking a half-word fill pattern.
		PatternSegment(DataSource & source, uint16_t pattern);
		
		//! \brief Constructor taking a word fill pattern.
		PatternSegment(DataSource & source, uint32_t pattern);

		//! \name Segment methods
		//@{
		//! \brief Pattern segments have no natural address.
		virtual bool hasNaturalLocation() { return false; }
		
		//! \brief Performs a pattern fill into the \a buffer.
		virtual unsigned getData(unsigned offset, unsigned maxBytes, uint8_t * buffer);
		
		//! \brief Returns a length based on the data target's address range.
		virtual unsigned getLength();
		//@}
		
		//! \name Pattern accessors
		//@{
		//! \brief Assigns a new fill pattern.
		inline void setPattern(const SizedIntegerValue & newPattern) { m_pattern = newPattern; }
		
		//! \brief Return the fill pattern for the segment.
		inline SizedIntegerValue & getPattern() { return m_pattern; }
	
		//! \brief Assignment operator, sets the pattern value and length.
		PatternSegment & operator = (const SizedIntegerValue & value) { m_pattern = value; return *this; }
		//@}
		
	protected:
		SizedIntegerValue m_pattern;	//!< The fill pattern.
	};
	
public:
	//! \brief Default constructor.
	DataSource() : m_target(0) {}
	
	//! \brief Destructor.
	virtual ~DataSource() {}
	
	//! \name Data target
	//@{
	//! \brief Sets the associated data target.
	inline void setTarget(DataTarget * target) { m_target = target; }
	
	//! \brief Gets the associated data target.
	inline DataTarget * getTarget() const { return m_target; }
	//@}
	
	//! \name Segments
	//@{
	//! \brief Returns the number of segments in this data source.
	virtual unsigned getSegmentCount()=0;
	
	//! \brief Returns segment number \a index of the data source.
	virtual Segment * getSegmentAt(unsigned index)=0;
	//@}
	
protected:
	DataTarget * m_target;	//!< Corresponding target for this source.
};

/*!
 * \brief Data source for a repeating pattern.
 *
 * The pattern is represented by a SizedIntegerValue object. Thus the pattern
 * can be either byte, half-word, or word sized.
 *
 * This data source has only one segment, and the PatternSource instance acts
 * as its own single segment.
 */
class PatternSource : public DataSource, public DataSource::PatternSegment
{
public:
	//! \brief Default constructor.
	PatternSource();
	
	//! \brief Constructor taking the pattern value.
	PatternSource(const SizedIntegerValue & value);
	
	//! \brief There is only one segment.
	virtual unsigned getSegmentCount() { return 1; }
	
	//! \brief Returns this object, as it is its own segment.
	virtual DataSource::Segment * getSegmentAt(unsigned index) { return this; }
	
	//! \brief Assignment operator, sets the pattern value and length.
	PatternSource & operator = (const SizedIntegerValue & value) { setPattern(value); return *this; }
};

/*!
 * \brief Data source for data that is not memory mapped (has no natural address).
 *
 * This data source can only manage a single block of data, which has no
 * associated address. It acts as its own Segment.
 */
class UnmappedDataSource : public DataSource, public DataSource::Segment
{
public:
	//! \brief Default constructor.
	UnmappedDataSource();
	
	//! \brief Constructor taking the data, which is copied.
	UnmappedDataSource(const uint8_t * data, unsigned length);
	
	//! \brief Sets the source's data.
	void setData(const uint8_t * data, unsigned length);
	
	//! \brief There is only one segment.
	virtual unsigned getSegmentCount() { return 1; }
	
	//! \brief Returns this object, as it is its own segment.
	virtual DataSource::Segment * getSegmentAt(unsigned index) { return this; }
		
	//! \name Segment methods
	//@{
	//! \brief Unmapped data sources have no natural address.
	virtual bool hasNaturalLocation() { return false; }
	
	//! \brief Copies a portion of the data into \a buffer.
	virtual unsigned getData(unsigned offset, unsigned maxBytes, uint8_t * buffer);
	
	//! \brief Returns the number of bytes of data managed by the source.
	virtual unsigned getLength() { return m_length; }
	//@}

protected:
	smart_array_ptr<uint8_t> m_data;	//!< The data.
	unsigned m_length;	//!< Byte count of the data.
};

/*!
 * \brief Data source that takes its data from an executable image.
 *
 * \see StExecutableImage
 */
class MemoryImageDataSource : public DataSource
{
public:
	//! \brief Default constructor.
	MemoryImageDataSource(StExecutableImage * image);
	
	//! \brief Destructor.
	virtual ~MemoryImageDataSource();
	
	//! \brief Returns the number of memory regions in the image.
	virtual unsigned getSegmentCount();
	
	//! \brief Returns the data source segment at position \a index.
	virtual DataSource::Segment * getSegmentAt(unsigned index);
	
protected:
	/*!
	 * \brief Segment corresponding to a text region of the executable image.
	 */
	class TextSegment : public DataSource::Segment
	{
	public:
		//! \brief Default constructor
		TextSegment(MemoryImageDataSource & source, StExecutableImage * image, unsigned index);
		
		virtual unsigned getData(unsigned offset, unsigned maxBytes, uint8_t * buffer);
		virtual unsigned getLength();
	
		virtual bool hasNaturalLocation() { return true; }
		virtual uint32_t getBaseAddress();
	
	protected:
		StExecutableImage * m_image;	//!< Coalesced image of the file.
		unsigned m_index;	//!< Record index.
	};
	
	/*!
	 * \brief Segment corresponding to a fill region of the executable image.
	 */
	class FillSegment : public DataSource::PatternSegment
	{
	public:
		FillSegment(MemoryImageDataSource & source, StExecutableImage * image, unsigned index);
		
		virtual unsigned getLength();
	
		virtual bool hasNaturalLocation() { return true; }
		virtual uint32_t getBaseAddress();
	
	protected:
		StExecutableImage * m_image;	//!< Coalesced image of the file.
		unsigned m_index;	//!< Record index.
	};
	
protected:
	StExecutableImage * m_image;	//!< The memory image that is the data source.
	
	typedef std::vector<DataSource::Segment*> segment_array_t;	//!< An array of segments.
	segment_array_t m_segments;	//!< The array of Segment instances.
};

}; // namespace elftosb

#endif // _DataSource_h_
