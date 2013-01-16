/*
 * File:	EvalContext.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_EvalContext_h_)
#define _EvalContext_h_

#include <map>
#include <string>
#include "Value.h"
#include "int_size.h"
#include "SourceFile.h"

namespace elftosb
{

/*!
 * \brief Context for evaluating AST tree and expressions.
 *
 * Keeps a map of variable names to integer values. Each integer value has a
 * size attribute in addition to the actual value. Variables can be locked, which
 * simply means that they cannot be assigned a new value.
 *
 * \todo Switch to using Value instances to keep track of variable values. This
 *		will enable variables to have string values, for one.
 */
class EvalContext
{
public:
	/*!
	 * \brief Abstract interface for a manager of source files.
	 */
	class SourceFileManager
	{
	public:
		//! \brief Returns true if a source file with the name \a name exists.
		virtual bool hasSourceFile(const std::string & name)=0;
		
		//! \brief Gets the requested source file.
		virtual SourceFile * getSourceFile(const std::string & name)=0;
		
		//! \brief Returns the default source file, or NULL if none is set.
		virtual SourceFile * getDefaultSourceFile()=0;
	};
	
public:
	//! \brief Constructor.
	EvalContext();
	
	//! \brief Destructor.
	virtual ~EvalContext();
	
	//! \name Source file manager
	//@{
	//! \brief
	void setSourceFileManager(SourceFileManager * manager) { m_sourcesManager = manager; }
	
	//! \brief
	SourceFileManager * getSourceFileManager() { return m_sourcesManager; }
	//@}
	
	//! \name Variables
	//@{
	bool isVariableDefined(const std::string & name);
	uint32_t getVariableValue(const std::string & name);
	int_size_t getVariableSize(const std::string & name);
	void setVariable(const std::string & name, uint32_t value, int_size_t size=kWordSize);
	//@}
	
	//! \name Locks
	//@{
	bool isVariableLocked(const std::string & name);
	void lockVariable(const std::string & name);
	void unlockVariable(const std::string & name);
	//@}
	
	void dump();

protected:
	//! Information about a variable's value.
	struct variable_info_t
	{
		uint32_t m_value;	//!< Variable value.
		int_size_t m_size;	//!< Number of bytes
		bool m_isLocked;	//!< Can this variable's value be changed?
	};
	
	//! Type to maps between the variable name and its info.
	typedef std::map<std::string, variable_info_t> variable_map_t;
	
	SourceFileManager * m_sourcesManager; //!< Interface to source file manager.
	variable_map_t m_variables;	//!< Map of variables to their final values.
};

}; // namespace elftosb

#endif // _EvalContext_h_
