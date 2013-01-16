/*
 * File:	EvalContext.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "EvalContext.h"
#include <stdexcept>
#include "format_string.h"

using namespace elftosb;

EvalContext::EvalContext()
:	m_sourcesManager(0)
{
}

EvalContext::~EvalContext()
{
}

bool EvalContext::isVariableDefined(const std::string & name)
{
	variable_map_t::const_iterator it = m_variables.find(name);
	return it != m_variables.end();
}

uint32_t EvalContext::getVariableValue(const std::string & name)
{
	variable_map_t::const_iterator it = m_variables.find(name);
	if (it == m_variables.end())
	{
		throw std::runtime_error(format_string("undefined variable '%s'", name.c_str()));
	}
	
	return it->second.m_value;
}

int_size_t EvalContext::getVariableSize(const std::string & name)
{
	variable_map_t::const_iterator it = m_variables.find(name);
	if (it == m_variables.end())
	{
		throw std::runtime_error(format_string("undefined variable '%s'", name.c_str()));
	}
	
	return it->second.m_size;
}

void EvalContext::setVariable(const std::string & name, uint32_t value, int_size_t size)
{
	// check if var is locked
	variable_map_t::const_iterator it = m_variables.find(name);
	if (it != m_variables.end() && it->second.m_isLocked)
	{
		return;
	}
	
	// set var info
	variable_info_t info;
	info.m_value = value;
	info.m_size = size;
	info.m_isLocked = false;
	m_variables[name] = info;
}

bool EvalContext::isVariableLocked(const std::string & name)
{
	variable_map_t::const_iterator it = m_variables.find(name);
	if (it == m_variables.end())
	{
		throw std::runtime_error(format_string("undefined variable '%s'", name.c_str()));
	}
	
	return it->second.m_isLocked;
}

void EvalContext::lockVariable(const std::string & name)
{
	variable_map_t::iterator it = m_variables.find(name);
	if (it == m_variables.end())
	{
		throw std::runtime_error(format_string("undefined variable '%s'", name.c_str()));
	}
	
	it->second.m_isLocked = true;
}

void EvalContext::unlockVariable(const std::string & name)
{
	variable_map_t::iterator it = m_variables.find(name);
	if (it == m_variables.end())
	{
		throw std::runtime_error("undefined variable");
	}
	
	it->second.m_isLocked = false;
}

void EvalContext::dump()
{
	variable_map_t::iterator it = m_variables.begin();
	for (; it != m_variables.end(); ++it)
	{
		variable_info_t & info = it->second;
		const char * lockedString = info.m_isLocked ? "locked" : "unlocked";
		printf("%s = %u:%d (%s)\n", it->first.c_str(), info.m_value, (int)info.m_size, lockedString);
	}
}

