/*
 * File:	Operation.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_Operation_h_)
#define _Operation_h_

#include "stdafx.h"
#include <vector>
#include "DataSource.h"
#include "DataTarget.h"
#include "smart_ptr.h"

namespace elftosb
{

/*!
 * \brief Abstract base class for all boot operations.
 */
class Operation
{
public:
	Operation() {}
	virtual ~Operation() {}
};

/*!
 * \brief Load data into memory operation.
 */
class LoadOperation : public Operation
{
public:
	LoadOperation() : Operation(), m_source(), m_target() {}
	
	void setSource(DataSource * source);
	inline DataSource * getSource() { return m_source; }
	
	void setTarget(DataTarget * target);
	inline DataTarget * getTarget() { return m_target; }
	
	inline void setDCDLoad(bool isDCD) { m_isDCDLoad = isDCD; }
	inline bool isDCDLoad() const { return m_isDCDLoad; }
	
protected:
	smart_ptr<DataSource> m_source;
	smart_ptr<DataTarget> m_target;
	bool m_isDCDLoad;
};

/*!
 * \brief Operation to execute code at a certain address.
 */
class ExecuteOperation : public Operation
{
public:
	enum execute_type_t
	{
		kJump,
		kCall
	};
	
public:
	ExecuteOperation() : Operation(), m_target(), m_argument(0), m_type(kCall), m_isHAB(false) {}

	inline void setTarget(DataTarget * target) { m_target = target; }
	inline DataTarget * getTarget() { return m_target; }
	
	inline void setArgument(uint32_t arg) { m_argument = arg; }
	inline uint32_t getArgument() { return m_argument; }
	
	inline void setExecuteType(execute_type_t type) { m_type = type; }
	inline execute_type_t getExecuteType() { return m_type; }
	
	inline void setIsHAB(bool isHAB) { m_isHAB = isHAB; }
	inline bool isHAB() const { return m_isHAB; }
	
protected:
	smart_ptr<DataTarget> m_target;
	uint32_t m_argument;
	execute_type_t m_type;
	bool m_isHAB;
};

/*!
 * \brief Authenticate with HAB and execute the entry point.
 */
class HABExecuteOperation : public ExecuteOperation
{
public:
	HABExecuteOperation() : ExecuteOperation() {}
};

/*!
 * \brief Operation to switch boot modes.
 */
class BootModeOperation : public Operation
{
public:
	BootModeOperation() : Operation() {}
	
	inline void setBootMode(uint32_t mode) { m_bootMode = mode; }
	inline uint32_t getBootMode() const { return m_bootMode; }

protected:
	uint32_t m_bootMode;	//!< The new boot mode value.
};

/*!
 * \brief Ordered sequence of operations.
 *
 * The operation objects owned by the sequence are \e not deleted when the
 * sequence is destroyed. The owner of the sequence must manually delete
 * the operation objects.
 */
class OperationSequence
{
public:
	typedef std::vector<Operation*> operation_list_t;	//!< Type for a list of operation objects.
	typedef operation_list_t::iterator iterator_t;	//!< Iterator over operations.
	typedef operation_list_t::const_iterator const_iterator_t;	//!< Const iterator over operations.

public:
	//! \brief Default constructor.
	OperationSequence() {}
	
	//! \brief Constructor. Makes a one-element sequence from \a soleElement.
	OperationSequence(Operation * soleElement) { m_operations.push_back(soleElement); }
	
	//! \brief Destructor.
	virtual ~OperationSequence();
	
	//! \name Iterators
	//@{
	inline iterator_t begin() { return m_operations.begin(); }
	inline const_iterator_t begin() const { return m_operations.begin(); }
	inline iterator_t end() { return m_operations.end(); }
	inline const_iterator_t end() const { return m_operations.end(); }
	//@}
	
	inline Operation * operator [] (unsigned index) const { return m_operations[index]; }
	
	//! \name Status
	//@{
	//! \brief Returns the number of operations in the sequence.
	inline unsigned getCount() const { return m_operations.size(); }
	//@}
	
	//! \name Operations
	//@{
	//! \brief Append one operation object to the sequence.
	inline void append(Operation * op) { m_operations.push_back(op); }
	
	//! \brief Append the contents of \a other onto this sequence.
	void append(const OperationSequence * other);
	
	//! \brief Appends \a other onto this sequence.
	OperationSequence & operator += (const OperationSequence * other) { append(other); return *this; }
	//@}

protected:
	operation_list_t m_operations;	//!< The list of operations.
};

}; // namespace elftosb

#endif // _Operation_h_
