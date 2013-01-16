/*
 * File:	Operation.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "Operation.h"

using namespace elftosb;

//! The operation object takes ownership of \a source.
//!
//! Cross references between the target and source are updated.
void LoadOperation::setSource(DataSource * source)
{
	m_source = source;
	
	if (m_target)
	{
		m_target->setSource(m_source);
	}
	if (m_source)
	{
		m_source->setTarget(m_target);
	}
}

//! The operation object takes ownership of \a target.
//!
//! Cross references between the target and source are updated.
void LoadOperation::setTarget(DataTarget * target)
{
	m_target = target;
	
	if (m_target)
	{
		m_target->setSource(m_source);
	}
	if (m_source)
	{
		m_source->setTarget(m_target);
	}
}

//! Disposes of operations objects in the sequence.
OperationSequence::~OperationSequence()
{
//	iterator_t it = begin();
//	for (; it != end(); ++it)
//	{
//		delete it->second;
//	}
}

void OperationSequence::append(const OperationSequence * other)
{
	const_iterator_t it = other->begin();
	for (; it != other->end(); ++it)
	{
		m_operations.push_back(*it);
	}
}
