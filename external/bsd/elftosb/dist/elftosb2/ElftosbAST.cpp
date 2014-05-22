/*
 * File:	ElftosbAST.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "ElftosbAST.h"
#include <stdexcept>
#include <math.h>
#include <assert.h>
#include "ElftosbErrors.h"
#include "format_string.h"

using namespace elftosb;

#pragma mark = ASTNode =

void ASTNode::printTree(int indent) const
{
	printIndent(indent);
	printf("%s\n", nodeName().c_str());
}

void ASTNode::printIndent(int indent) const
{
	int i;
	for (i=0; i<indent; ++i)
	{
		printf("   ");
	}
}

void ASTNode::setLocation(token_loc_t & first, token_loc_t & last)
{
	m_location.m_firstLine = first.m_firstLine;
	m_location.m_lastLine = last.m_lastLine;
}

void ASTNode::setLocation(ASTNode * first, ASTNode * last)
{
	m_location.m_firstLine = first->getLocation().m_firstLine;
	m_location.m_lastLine = last->getLocation().m_lastLine;
}

#pragma mark = ListASTNode =

ListASTNode::ListASTNode(const ListASTNode & other)
:	ASTNode(other), m_list()
{
	// deep copy each item of the original's list
	const_iterator it = other.begin();
	for (; it != other.end(); ++it)
	{
		m_list.push_back((*it)->clone());
	}
}

//! Deletes child node in the list.
//!
ListASTNode::~ListASTNode()
{
	iterator it = begin();
	for (; it != end(); it++)
	{
		delete *it;
	}
}

//! If \a node is NULL then the list is left unmodified.
//!
//! The list node's location is automatically updated after the node is added by a call
//! to updateLocation().
void ListASTNode::appendNode(ASTNode * node)
{
	if (node)
	{
		m_list.push_back(node);
		updateLocation();
	}
}

void ListASTNode::printTree(int indent) const
{
	ASTNode::printTree(indent);
	
	int n = 0;
	const_iterator it = begin();
	for (; it != end(); it++, n++)
	{
		printIndent(indent + 1);
		printf("%d:\n", n);
		(*it)->printTree(indent + 2);
	}
}

void ListASTNode::updateLocation()
{
	token_loc_t current = { 0 };
	const_iterator it = begin();
	for (; it != end(); it++)
	{
		const ASTNode * node = *it;
		const token_loc_t & loc = node->getLocation();
		
		// handle first node
		if (current.m_firstLine == 0)
		{
			current = loc;
			continue;
		}

		if (loc.m_firstLine < current.m_firstLine)
		{
			current.m_firstLine = loc.m_firstLine;
		}
		
		if (loc.m_lastLine > current.m_lastLine)
		{
			current.m_lastLine = loc.m_lastLine;
		}
	}
	
	setLocation(current);
}

#pragma mark = CommandFileASTNode =

CommandFileASTNode::CommandFileASTNode()
:	ASTNode(), m_options(), m_constants(), m_sources(), m_sections()
{
}

CommandFileASTNode::CommandFileASTNode(const CommandFileASTNode & other)
:	ASTNode(other), m_options(), m_constants(), m_sources(), m_sections()
{
	m_options = dynamic_cast<ListASTNode*>(other.m_options->clone());
	m_constants = dynamic_cast<ListASTNode*>(other.m_constants->clone());
	m_sources = dynamic_cast<ListASTNode*>(other.m_sources->clone());
	m_sections = dynamic_cast<ListASTNode*>(other.m_sections->clone());
}

void CommandFileASTNode::printTree(int indent) const
{
	ASTNode::printTree(indent);
	
	printIndent(indent + 1);
	printf("options:\n");
	if (m_options) m_options->printTree(indent + 2);
	
	printIndent(indent + 1);
	printf("constants:\n");
	if (m_constants) m_constants->printTree(indent + 2);
	
	printIndent(indent + 1);
	printf("sources:\n");
	if (m_sources) m_sources->printTree(indent + 2);
	
	printIndent(indent + 1);
	printf("sections:\n");
	if (m_sections) m_sections->printTree(indent + 2);
}

#pragma mark = ExprASTNode =

int_size_t ExprASTNode::resultIntSize(int_size_t a, int_size_t b)
{
	int_size_t result;
	switch (a)
	{
		case kWordSize:
			result = kWordSize;
			break;
		case kHalfWordSize:
			if (b == kWordSize)
			{
				result = kWordSize;
			}
			else
			{
				result = kHalfWordSize;
			}
			break;
		case kByteSize:
			if (b == kWordSize)
			{
				result = kWordSize;
			}
			else if (b == kHalfWordSize)
			{
				result = kHalfWordSize;
			}
			else
			{
				result = kByteSize;
			}
			break;
	}
	
	return result;
}

#pragma mark = IntConstExprASTNode =

IntConstExprASTNode::IntConstExprASTNode(const IntConstExprASTNode & other)
:	ExprASTNode(other), m_value(other.m_value), m_size(other.m_size)
{
}

void IntConstExprASTNode::printTree(int indent) const
{
	printIndent(indent);
	char sizeChar='?';
	switch (m_size)
	{
		case kWordSize:
			sizeChar = 'w';
			break;
		case kHalfWordSize:
			sizeChar = 'h';
			break;
		case kByteSize:
			sizeChar = 'b';
			break;
	}
	printf("%s(%d:%c)\n", nodeName().c_str(), m_value, sizeChar);
}

#pragma mark = VariableExprASTNode =

VariableExprASTNode::VariableExprASTNode(const VariableExprASTNode & other)
:	ExprASTNode(other), m_variable()
{
	m_variable = new std::string(*other.m_variable);
}

void VariableExprASTNode::printTree(int indent) const
{
	printIndent(indent);
	printf("%s(%s)\n", nodeName().c_str(), m_variable->c_str());
}

ExprASTNode * VariableExprASTNode::reduce(EvalContext & context)
{
	if (!context.isVariableDefined(*m_variable))
	{
		throw std::runtime_error(format_string("line %d: undefined variable '%s'", getFirstLine(), m_variable->c_str()));
	}
	
	uint32_t value = context.getVariableValue(*m_variable);
	int_size_t size = context.getVariableSize(*m_variable);
	return new IntConstExprASTNode(value, size);
}

#pragma mark = SymbolRefExprASTNode =

SymbolRefExprASTNode::SymbolRefExprASTNode(const SymbolRefExprASTNode & other)
:	ExprASTNode(other), m_symbol(NULL)
{
	if (other.m_symbol)
	{
		m_symbol = dynamic_cast<SymbolASTNode*>(other.m_symbol->clone());
	}
}

void SymbolRefExprASTNode::printTree(int indent) const
{
}

ExprASTNode * SymbolRefExprASTNode::reduce(EvalContext & context)
{
	EvalContext::SourceFileManager * manager = context.getSourceFileManager();
	if (!manager)
	{
		throw std::runtime_error("no source manager available");
	}
	
	if (!m_symbol)
	{
		throw semantic_error("no symbol provided");
	}
	
	// Get the name of the symbol
	std::string * symbolName = m_symbol->getSymbolName();
//	if (!symbolName)
//	{
//		throw semantic_error(format_string("line %d: no symbol name provided", getFirstLine()));
//	}
	
	// Get the source file.
	std::string * sourceName = m_symbol->getSource();
	SourceFile * sourceFile;
	
	if (sourceName)
	{
		sourceFile = manager->getSourceFile(*sourceName);
		if (!sourceFile)
		{
			throw semantic_error(format_string("line %d: no source file named %s", getFirstLine(), sourceName->c_str()));
		}
	}
	else
	{
		sourceFile = manager->getDefaultSourceFile();
		if (!sourceFile)
		{
			throw semantic_error(format_string("line %d: no default source file is set", getFirstLine()));
		}
	}
	
	// open the file if it hasn't already been
	if (!sourceFile->isOpen())
	{
		sourceFile->open();
	}
	
	// Make sure the source file supports symbols before going any further
	if (symbolName && !sourceFile->supportsNamedSymbols())
	{
		throw semantic_error(format_string("line %d: source file %s does not support symbols", getFirstLine(), sourceFile->getPath().c_str()));
	}
    
    if (!symbolName && !sourceFile->hasEntryPoint())
    {
        throw semantic_error(format_string("line %d: source file %s does not have an entry point", getFirstLine(), sourceFile->getPath().c_str()));
    }
	
	// Returns a const expr node with the symbol's value.
	uint32_t value;
    if (symbolName)
    {
        value = sourceFile->getSymbolValue(*symbolName);
    }
    else
    {
        value = sourceFile->getEntryPointAddress();
    }
	return new IntConstExprASTNode(value);
}

#pragma mark = NegativeExprASTNode =

NegativeExprASTNode::NegativeExprASTNode(const NegativeExprASTNode & other)
:	ExprASTNode(other), m_expr()
{
	m_expr = dynamic_cast<ExprASTNode*>(other.m_expr->clone());
}

void NegativeExprASTNode::printTree(int indent) const
{
	ExprASTNode::printTree(indent);
	if (m_expr) m_expr->printTree(indent + 1);
}

ExprASTNode * NegativeExprASTNode::reduce(EvalContext & context)
{
	if (!m_expr)
	{
		return this;
	}
	
	m_expr = m_expr->reduce(context);
	IntConstExprASTNode * intConst = dynamic_cast<IntConstExprASTNode*>(m_expr.get());
	if (intConst)
	{
	    int32_t value = -(int32_t)intConst->getValue();
		return new IntConstExprASTNode((uint32_t)value, intConst->getSize());
	}
	else
	{
		return this;
	}
}

#pragma mark = BooleanNotExprASTNode =

BooleanNotExprASTNode::BooleanNotExprASTNode(const BooleanNotExprASTNode & other)
:	ExprASTNode(other), m_expr()
{
	m_expr = dynamic_cast<ExprASTNode*>(other.m_expr->clone());
}

void BooleanNotExprASTNode::printTree(int indent) const
{
	ExprASTNode::printTree(indent);
	if (m_expr) m_expr->printTree(indent + 1);
}

ExprASTNode * BooleanNotExprASTNode::reduce(EvalContext & context)
{
	if (!m_expr)
	{
		return this;
	}
	
	m_expr = m_expr->reduce(context);
	IntConstExprASTNode * intConst = dynamic_cast<IntConstExprASTNode*>(m_expr.get());
	if (intConst)
	{
	    int32_t value = !((int32_t)intConst->getValue());
		return new IntConstExprASTNode((uint32_t)value, intConst->getSize());
	}
	else
	{
		throw semantic_error(format_string("line %d: expression did not evaluate to an integer", m_expr->getFirstLine()));
	}
}

#pragma mark = SourceFileFunctionASTNode =

SourceFileFunctionASTNode::SourceFileFunctionASTNode(const SourceFileFunctionASTNode & other)
:	ExprASTNode(other), m_functionName(), m_sourceFile()
{
	m_functionName = new std::string(*other.m_functionName);
	m_sourceFile = new std::string(*other.m_sourceFile);
}

void SourceFileFunctionASTNode::printTree(int indent) const
{
	ExprASTNode::printTree(indent);
	printIndent(indent+1);

	// for some stupid reason the msft C++ compiler barfs on the following line if the ".get()" parts are remove,
	// even though the first line of reduce() below has the same expression, just in parentheses. stupid compiler.
	if (m_functionName.get() && m_sourceFile.get())
	{
		printf("%s ( %s )\n", m_functionName->c_str(), m_sourceFile->c_str());
	}
}

ExprASTNode * SourceFileFunctionASTNode::reduce(EvalContext & context)
{
	if (!(m_functionName && m_sourceFile))
	{
		throw std::runtime_error("unset function name or source file");
	}
	
	// Get source file manager from evaluation context. This will be the
	// conversion controller itself.
	EvalContext::SourceFileManager * mgr = context.getSourceFileManager();
	if (!mgr)
	{
		throw std::runtime_error("source file manager is not set");
	}
	
	// Perform function
	uint32_t functionResult = 0;
	if (*m_functionName == "exists")
	{
		functionResult = static_cast<uint32_t>(mgr->hasSourceFile(*m_sourceFile));
	}
	
	// Return function result as an expression node
	return new IntConstExprASTNode(functionResult);
}

#pragma mark = DefinedOperatorASTNode =

DefinedOperatorASTNode::DefinedOperatorASTNode(const DefinedOperatorASTNode & other)
:	ExprASTNode(other), m_constantName()
{
	m_constantName = new std::string(*other.m_constantName);
}

void DefinedOperatorASTNode::printTree(int indent) const
{
	ExprASTNode::printTree(indent);
	printIndent(indent+1);

	if (m_constantName)
	{
		printf("defined ( %s )\n", m_constantName->c_str());
	}
}

ExprASTNode * DefinedOperatorASTNode::reduce(EvalContext & context)
{
	assert(m_constantName);
	
	// Return function result as an expression node
	return new IntConstExprASTNode(context.isVariableDefined(m_constantName) ? 1 : 0);
}

#pragma mark = SizeofOperatorASTNode =

SizeofOperatorASTNode::SizeofOperatorASTNode(const SizeofOperatorASTNode & other)
:	ExprASTNode(other), m_constantName(), m_symbol()
{
	m_constantName = new std::string(*other.m_constantName);
	m_symbol = dynamic_cast<SymbolASTNode*>(other.m_symbol->clone());
}

void SizeofOperatorASTNode::printTree(int indent) const
{
	ExprASTNode::printTree(indent);
	
	printIndent(indent+1);

	if (m_constantName)
	{
		printf("sizeof: %s\n", m_constantName->c_str());
	}
	else if (m_symbol)
	{
		printf("sizeof:\n");
		m_symbol->printTree(indent + 2);
	}
}

ExprASTNode * SizeofOperatorASTNode::reduce(EvalContext & context)
{
	// One or the other must be defined.
	assert(m_constantName || m_symbol);
	
	EvalContext::SourceFileManager * manager = context.getSourceFileManager();
	assert(manager);
	
	unsigned sizeInBytes = 0;
	SourceFile * sourceFile;
	
	if (m_symbol)
	{
		// Get the symbol name.
		std::string * symbolName = m_symbol->getSymbolName();
		assert(symbolName);
		
		// Get the source file, using the default if one is not specified.
		std::string * sourceName = m_symbol->getSource();
		if (sourceName)
		{
			sourceFile = manager->getSourceFile(*sourceName);
			if (!sourceFile)
			{
				throw semantic_error(format_string("line %d: invalid source file: %s", getFirstLine(), sourceName->c_str()));
			}
		}
		else
		{
			sourceFile = manager->getDefaultSourceFile();
			if (!sourceFile)
			{
				throw semantic_error(format_string("line %d: no default source file is set", getFirstLine()));
			}
		}
		
		// Get the size of the symbol.
		if (sourceFile->hasSymbol(*symbolName))
		{
			sizeInBytes = sourceFile->getSymbolSize(*symbolName);
		}
	}
	else if (m_constantName)
	{
		// See if the "constant" is really a constant or if it's a source name.
		if (manager->hasSourceFile(m_constantName))
		{
			sourceFile = manager->getSourceFile(m_constantName);
			if (sourceFile)
			{
				sizeInBytes = sourceFile->getSize();
			}
		}
		else
		{
			// Regular constant.
			if (!context.isVariableDefined(*m_constantName))
			{
				throw semantic_error(format_string("line %d: cannot get size of undefined constant %s", getFirstLine(), m_constantName->c_str()));
			}
			
			int_size_t intSize = context.getVariableSize(*m_constantName);
			switch (intSize)
			{
				case kWordSize:
					sizeInBytes = sizeof(uint32_t);
					break;
				case kHalfWordSize:
					sizeInBytes = sizeof(uint16_t);
					break;
				case kByteSize:
					sizeInBytes = sizeof(uint8_t);
					break;
			}
		}
	}
	
	// Return function result as an expression node
	return new IntConstExprASTNode(sizeInBytes);
}

#pragma mark = BinaryOpExprASTNode =

BinaryOpExprASTNode::BinaryOpExprASTNode(const BinaryOpExprASTNode & other)
:	ExprASTNode(other), m_left(), m_op(other.m_op), m_right()
{
	m_left = dynamic_cast<ExprASTNode*>(other.m_left->clone());
	m_right = dynamic_cast<ExprASTNode*>(other.m_right->clone());
}

void BinaryOpExprASTNode::printTree(int indent) const
{
	ExprASTNode::printTree(indent);

	printIndent(indent + 1);
	printf("left:\n");
	if (m_left) m_left->printTree(indent + 2);
	
	printIndent(indent + 1);
	printf("op: %s\n", getOperatorName().c_str());

	printIndent(indent + 1);
	printf("right:\n");
	if (m_right) m_right->printTree(indent + 2);
}

std::string BinaryOpExprASTNode::getOperatorName() const
{
	switch (m_op)
	{
		case kAdd:
			return "+";
		case kSubtract:
			return "-";
		case kMultiply:
			return "*";
		case kDivide:
			return "/";
		case kModulus:
			return "%";
		case kPower:
			return "**";
		case kBitwiseAnd:
			return "&";
		case kBitwiseOr:
			return "|";
		case kBitwiseXor:
			return "^";
		case kShiftLeft:
			return "<<";
		case kShiftRight:
			return ">>";
		case kLessThan:
			return "<";
		case kGreaterThan:
			return ">";
		case kLessThanEqual:
			return "<=";
		case kGreaterThanEqual:
			return ">";
		case kEqual:
			return "==";
		case kNotEqual:
			return "!=";
		case kBooleanAnd:
			return "&&";
		case kBooleanOr:
			return "||";
	}
	
	return "???";
}

//! \todo Fix power operator under windows!!!
//!
ExprASTNode * BinaryOpExprASTNode::reduce(EvalContext & context)
{
	if (!m_left || !m_right)
	{
		return this;
	}
	
	IntConstExprASTNode * leftIntConst = NULL;
	IntConstExprASTNode * rightIntConst = NULL;
	uint32_t leftValue;
	uint32_t rightValue;
	uint32_t result = 0;
	
	// Always reduce the left hand side.
	m_left = m_left->reduce(context);
	leftIntConst = dynamic_cast<IntConstExprASTNode*>(m_left.get());
	if (!leftIntConst)
	{
		throw semantic_error(format_string("left hand side of %s operator failed to evaluate to an integer", getOperatorName().c_str()));
	}
	leftValue = leftIntConst->getValue();
	
	// Boolean && and || operators are handled separately so that we can perform
	// short-circuit evaluation.
	if (m_op == kBooleanAnd || m_op == kBooleanOr)
	{
		// Reduce right hand side only if required to evaluate the boolean operator.
		if ((m_op == kBooleanAnd && leftValue != 0) || (m_op == kBooleanOr && leftValue == 0))
		{
			m_right = m_right->reduce(context);
			rightIntConst = dynamic_cast<IntConstExprASTNode*>(m_right.get());
			if (!rightIntConst)
			{
				throw semantic_error(format_string("right hand side of %s operator failed to evaluate to an integer", getOperatorName().c_str()));
			}
			rightValue = rightIntConst->getValue();
			
			// Perform the boolean operation.
			switch (m_op)
			{
				case kBooleanAnd:
					result = leftValue && rightValue;
					break;
				
				case kBooleanOr:
					result = leftValue && rightValue;
					break;
			}
		}
		else if (m_op == kBooleanAnd)
		{
			// The left hand side is false, so the && operator's result must be false
			// without regard to the right hand side.
			result = 0;
		}
		else if (m_op == kBooleanOr)
		{
			// The left hand value is true so the || result is automatically true.
			result = 1;
		}
	}
	else
	{
		// Reduce right hand side always for most operators.
		m_right = m_right->reduce(context);
		rightIntConst = dynamic_cast<IntConstExprASTNode*>(m_right.get());
		if (!rightIntConst)
		{
			throw semantic_error(format_string("right hand side of %s operator failed to evaluate to an integer", getOperatorName().c_str()));
		}
		rightValue = rightIntConst->getValue();
		
		switch (m_op)
		{
			case kAdd:
				result = leftValue + rightValue;
				break;
			case kSubtract:
				result = leftValue - rightValue;
				break;
			case kMultiply:
				result = leftValue * rightValue;
				break;
			case kDivide:
				result = leftValue / rightValue;
				break;
			case kModulus:
				result = leftValue % rightValue;
				break;
			case kPower:
			#ifdef WIN32
				result = 0;
			#else
				result = lroundf(powf(float(leftValue), float(rightValue)));
			#endif
				break;
			case kBitwiseAnd:
				result = leftValue & rightValue;
				break;
			case kBitwiseOr:
				result = leftValue | rightValue;
				break;
			case kBitwiseXor:
				result = leftValue ^ rightValue;
				break;
			case kShiftLeft:
				result = leftValue << rightValue;
				break;
			case kShiftRight:
				result = leftValue >> rightValue;
				break;
			case kLessThan:
				result = leftValue < rightValue;
				break;
			case kGreaterThan:
				result = leftValue > rightValue;
				break;
			case kLessThanEqual:
				result = leftValue <= rightValue;
				break;
			case kGreaterThanEqual:
				result = leftValue >= rightValue;
				break;
			case kEqual:
				result = leftValue == rightValue;
				break;
			case kNotEqual:
				result = leftValue != rightValue;
				break;
		}
	}
	
	// Create the result value.
	int_size_t resultSize;
	if (leftIntConst && rightIntConst)
	{
		resultSize = resultIntSize(leftIntConst->getSize(), rightIntConst->getSize());
	}
	else if (leftIntConst)
	{
		resultSize = leftIntConst->getSize();
	}
	else
	{
		// This shouldn't really be possible, but just in case.
		resultSize = kWordSize;
	}
	return new IntConstExprASTNode(result, resultSize);
}

#pragma mark = IntSizeExprASTNode =

IntSizeExprASTNode::IntSizeExprASTNode(const IntSizeExprASTNode & other)
:	ExprASTNode(other), m_expr(), m_size(other.m_size)
{
	m_expr = dynamic_cast<ExprASTNode*>(other.m_expr->clone());
}

void IntSizeExprASTNode::printTree(int indent) const
{
	ExprASTNode::printTree(indent);
	
	char sizeChar='?';
	switch (m_size)
	{
		case kWordSize:
			sizeChar = 'w';
			break;
		case kHalfWordSize:
			sizeChar = 'h';
			break;
		case kByteSize:
			sizeChar = 'b';
			break;
	}
	printIndent(indent + 1);
	printf("size: %c\n", sizeChar);
	
	printIndent(indent + 1);
	printf("expr:\n");
	if (m_expr) m_expr->printTree(indent + 2);
}

ExprASTNode * IntSizeExprASTNode::reduce(EvalContext & context)
{
	if (!m_expr)
	{
		return this;
	}
	
	m_expr = m_expr->reduce(context);
	IntConstExprASTNode * intConst = dynamic_cast<IntConstExprASTNode*>(m_expr.get());
	if (!intConst)
	{
		return this;
	}
	
	return new IntConstExprASTNode(intConst->getValue(), m_size);
}

#pragma mark = ExprConstASTNode =

ExprConstASTNode::ExprConstASTNode(const ExprConstASTNode & other)
:	ConstASTNode(other), m_expr()
{
	m_expr = dynamic_cast<ExprASTNode*>(other.m_expr->clone());
}

void ExprConstASTNode::printTree(int indent) const
{
	ConstASTNode::printTree(indent);
	if (m_expr) m_expr->printTree(indent + 1);
}

#pragma mark = StringConstASTNode =

StringConstASTNode::StringConstASTNode(const StringConstASTNode & other)
:	ConstASTNode(other), m_value()
{
	m_value = new std::string(other.m_value);
}

void StringConstASTNode::printTree(int indent) const
{
	printIndent(indent);
	printf("%s(%s)\n", nodeName().c_str(), m_value->c_str());
}

#pragma mark = BlobConstASTNode =

BlobConstASTNode::BlobConstASTNode(const BlobConstASTNode & other)
:	ConstASTNode(other), m_blob()
{
	m_blob = new Blob(*other.m_blob);
}

void BlobConstASTNode::printTree(int indent) const
{
	printIndent(indent);
	
	const uint8_t * dataPtr = m_blob->getData();
	unsigned dataLen = m_blob->getLength();
	printf("%s(%p:%d)\n", nodeName().c_str(), dataPtr, dataLen);
}

#pragma mark = IVTConstASTNode =

IVTConstASTNode::IVTConstASTNode(const IVTConstASTNode & other)
:	ConstASTNode(other), m_fields()
{
	m_fields = dynamic_cast<ListASTNode*>(other.m_fields->clone());
}

void IVTConstASTNode::printTree(int indent) const
{
	printIndent(indent);
	printf("%s:\n", nodeName().c_str());
    if (m_fields)
    {
        m_fields->printTree(indent + 1);
    }
}

#pragma mark = AssignmentASTNode =

AssignmentASTNode::AssignmentASTNode(const AssignmentASTNode & other)
:	ASTNode(other), m_ident(), m_value()
{
	m_ident = new std::string(*other.m_ident);
	m_value = dynamic_cast<ConstASTNode*>(other.m_value->clone());
}

void AssignmentASTNode::printTree(int indent) const
{
	printIndent(indent);
	printf("%s(%s)\n", nodeName().c_str(), m_ident->c_str());
	
	if (m_value) m_value->printTree(indent + 1);
}

#pragma mark = SourceDefASTNode =

SourceDefASTNode::SourceDefASTNode(const SourceDefASTNode & other)
:	ASTNode(other), m_name()
{
	m_name = new std::string(*other.m_name);
}

#pragma mark = PathSourceDefASTNode =

PathSourceDefASTNode::PathSourceDefASTNode(const PathSourceDefASTNode & other)
:	SourceDefASTNode(other), m_path()
{
	m_path = new std::string(*other.m_path);
}

void PathSourceDefASTNode::printTree(int indent) const
{
	SourceDefASTNode::printTree(indent);
	
	printIndent(indent+1);
	printf("path: %s\n", m_path->c_str());
	
	printIndent(indent+1);
	printf("attributes:\n");
	if (m_attributes)
	{
		m_attributes->printTree(indent+2);
	}
}

#pragma mark = ExternSourceDefASTNode =

ExternSourceDefASTNode::ExternSourceDefASTNode(const ExternSourceDefASTNode & other)
:	SourceDefASTNode(other), m_expr()
{
	m_expr = dynamic_cast<ExprASTNode*>(other.m_expr->clone());
}

void ExternSourceDefASTNode::printTree(int indent) const
{
	SourceDefASTNode::printTree(indent);
	
	printIndent(indent+1);
	printf("expr:\n");
	if (m_expr) m_expr->printTree(indent + 2);
	
	printIndent(indent+1);
	printf("attributes:\n");
	if (m_attributes)
	{
		m_attributes->printTree(indent+2);
	}
}

#pragma mark = SectionContentsASTNode =

SectionContentsASTNode::SectionContentsASTNode(const SectionContentsASTNode & other)
:	ASTNode(other), m_sectionExpr()
{
	m_sectionExpr = dynamic_cast<ExprASTNode*>(other.m_sectionExpr->clone());
}

void SectionContentsASTNode::printTree(int indent) const
{
	ASTNode::printTree(indent);
	
	printIndent(indent + 1);
	printf("section#:\n");
	if (m_sectionExpr) m_sectionExpr->printTree(indent + 2);
}

#pragma mark = DataSectionContentsASTNode =

DataSectionContentsASTNode::DataSectionContentsASTNode(const DataSectionContentsASTNode & other)
:	SectionContentsASTNode(other), m_contents()
{
	m_contents = dynamic_cast<ASTNode*>(other.m_contents->clone());
}

void DataSectionContentsASTNode::printTree(int indent) const
{
	SectionContentsASTNode::printTree(indent);
	
	if (m_contents)
	{
		m_contents->printTree(indent + 1);
	}
}

#pragma mark = BootableSectionContentsASTNode =

BootableSectionContentsASTNode::BootableSectionContentsASTNode(const BootableSectionContentsASTNode & other)
:	SectionContentsASTNode(other), m_statements()
{
	m_statements = dynamic_cast<ListASTNode*>(other.m_statements->clone());
}

void BootableSectionContentsASTNode::printTree(int indent) const
{
	SectionContentsASTNode::printTree(indent);
	
	printIndent(indent + 1);
	printf("statements:\n");
	if (m_statements) m_statements->printTree(indent + 2);
}

#pragma mark = IfStatementASTNode =

//! \warning Be careful; this method could enter an infinite loop if m_nextIf feeds
//!		back onto itself. m_nextIf must be NULL at some point down the next if list.
IfStatementASTNode::IfStatementASTNode(const IfStatementASTNode & other)
:	StatementASTNode(),
	m_conditionExpr(),
	m_ifStatements(),
	m_nextIf(),
	m_elseStatements()
{
	m_conditionExpr = dynamic_cast<ExprASTNode*>(other.m_conditionExpr->clone());
	m_ifStatements = dynamic_cast<ListASTNode*>(other.m_ifStatements->clone());
	m_nextIf = dynamic_cast<IfStatementASTNode*>(other.m_nextIf->clone());
	m_elseStatements = dynamic_cast<ListASTNode*>(other.m_elseStatements->clone());
}

#pragma mark = ModeStatementASTNode =

ModeStatementASTNode::ModeStatementASTNode(const ModeStatementASTNode & other)
:	StatementASTNode(other), m_modeExpr()
{
	m_modeExpr = dynamic_cast<ExprASTNode*>(other.m_modeExpr->clone());
}

void ModeStatementASTNode::printTree(int indent) const
{
	StatementASTNode::printTree(indent);
	printIndent(indent + 1);
	printf("mode:\n");
	if (m_modeExpr) m_modeExpr->printTree(indent + 2);
}

#pragma mark = MessageStatementASTNode =

MessageStatementASTNode::MessageStatementASTNode(const MessageStatementASTNode & other)
:	StatementASTNode(other), m_type(other.m_type), m_message()
{
	m_message = new std::string(*other.m_message);
}

void MessageStatementASTNode::printTree(int indent) const
{
	StatementASTNode::printTree(indent);
	printIndent(indent + 1);
	printf("%s: %s\n", getTypeName(), m_message->c_str());
}

const char * MessageStatementASTNode::getTypeName() const
{
	switch (m_type)
	{
		case kInfo:
			return "info";
		
		case kWarning:
			return "warning";
		
		case kError:
			return "error";
	}
	
	return "unknown";
}

#pragma mark = LoadStatementASTNode =

LoadStatementASTNode::LoadStatementASTNode(const LoadStatementASTNode & other)
:	StatementASTNode(other), m_data(), m_target(), m_isDCDLoad(other.m_isDCDLoad)
{
	m_data = other.m_data->clone();
	m_target = other.m_target->clone();
}

void LoadStatementASTNode::printTree(int indent) const
{
	StatementASTNode::printTree(indent);
	
	printIndent(indent + 1);
	printf("data:\n");
	if (m_data) m_data->printTree(indent + 2);
	
	printIndent(indent + 1);
	printf("target:\n");
	if (m_target) m_target->printTree(indent + 2);
}

#pragma mark = CallStatementASTNode =

CallStatementASTNode::CallStatementASTNode(const CallStatementASTNode & other)
:	StatementASTNode(other), m_type(other.m_type), m_target(), m_arg()
{
	m_target = other.m_target->clone();
	m_arg = other.m_arg->clone();
}

void CallStatementASTNode::printTree(int indent) const
{
	printIndent(indent);
	printf("%s(%s)%s\n", nodeName().c_str(), (m_type == kCallType ? "call" : "jump"), (m_isHAB ? "/HAB" : ""));
	
	printIndent(indent + 1);
	printf("target:\n");
	if (m_target) m_target->printTree(indent + 2);
	
	printIndent(indent + 1);
	printf("arg:\n");
	if (m_arg) m_arg->printTree(indent + 2);
}

#pragma mark = SourceASTNode =

SourceASTNode::SourceASTNode(const SourceASTNode & other)
:	ASTNode(other), m_name()
{
	m_name = new std::string(*other.m_name);
}

void SourceASTNode::printTree(int indent) const
{
	printIndent(indent);
	printf("%s(%s)\n", nodeName().c_str(), m_name->c_str());
}

#pragma mark = SectionMatchListASTNode =

SectionMatchListASTNode::SectionMatchListASTNode(const SectionMatchListASTNode & other)
:	ASTNode(other), m_sections(), m_source()
{
	if (other.m_sections)
	{
		m_sections = dynamic_cast<ListASTNode *>(other.m_sections->clone());
	}
	
	if (other.m_source)
	{
		m_source = new std::string(*other.m_source);
	}
}

void SectionMatchListASTNode::printTree(int indent) const
{
	ASTNode::printTree(indent);
	
	printIndent(indent+1);
	printf("sections:\n");
	if (m_sections)
	{
		m_sections->printTree(indent+2);
	}
	
	printIndent(indent+1);
	printf("source: ");
	if (m_source)
	{
		printf("%s\n", m_source->c_str());
	}
	else
	{
		printf("\n");
	}
}

#pragma mark = SectionASTNode =

SectionASTNode::SectionASTNode(const SectionASTNode & other)
:	ASTNode(other), m_name(), m_source()
{
	m_action = other.m_action;
	
	if (other.m_name)
	{
		m_name = new std::string(*other.m_name);
	}
	
	if (other.m_source)
	{
		m_source = new std::string(*other.m_source);
	}
}

void SectionASTNode::printTree(int indent) const
{
	printIndent(indent);
	
	const char * actionName;
	switch (m_action)
	{
		case kInclude:
			actionName = "include";
			break;
		case kExclude:
			actionName = "exclude";
			break;
	}
	
	if (m_source)
	{
		printf("%s(%s:%s:%s)\n", nodeName().c_str(), actionName, m_name->c_str(), m_source->c_str());
	}
	else
	{
		printf("%s(%s:%s)\n", nodeName().c_str(), actionName, m_name->c_str());
	}
}

#pragma mark = SymbolASTNode =

SymbolASTNode::SymbolASTNode(const SymbolASTNode & other)
:	ASTNode(other), m_symbol(), m_source()
{
	m_symbol = new std::string(*other.m_symbol);
	m_source = new std::string(*other.m_source);
}

void SymbolASTNode::printTree(int indent) const
{
	printIndent(indent);
	
	const char * symbol = NULL;
	if (m_symbol)
	{
		symbol = m_symbol->c_str();
	}
	
	const char * source = NULL;
	if (m_source)
	{
		source = m_source->c_str();
	}
	
	printf("%s(", nodeName().c_str());
	if (source)
	{
		printf("%s", source);
	}
	else
	{
		printf(".");
	}
	printf(":");
	if (symbol)
	{
		printf("%s", symbol);
	}
	else
	{
		printf(".");
	}
	printf(")\n");
}

#pragma mark = AddressRangeASTNode =

AddressRangeASTNode::AddressRangeASTNode(const AddressRangeASTNode & other)
:	ASTNode(other), m_begin(), m_end()
{
	m_begin = other.m_begin->clone();
	m_end = other.m_end->clone();
}

void AddressRangeASTNode::printTree(int indent) const
{
	ASTNode::printTree(indent);
	
	printIndent(indent + 1);
	printf("begin:\n");
	if (m_begin) m_begin->printTree(indent + 2);
	
	printIndent(indent + 1);
	printf("end:\n");
	if (m_end) m_end->printTree(indent + 2);
}

#pragma mark = FromStatementASTNode =

FromStatementASTNode::FromStatementASTNode(std::string * source, ListASTNode * statements)
:	StatementASTNode(), m_source(source), m_statements(statements)
{
}

FromStatementASTNode::FromStatementASTNode(const FromStatementASTNode & other)
:	StatementASTNode(), m_source(), m_statements()
{
	m_source = new std::string(*other.m_source);
	m_statements = dynamic_cast<ListASTNode*>(other.m_statements->clone());
}

void FromStatementASTNode::printTree(int indent) const
{
	ASTNode::printTree(indent);
	
	printIndent(indent + 1);
	printf("source: ");
	if (m_source) printf("%s\n", m_source->c_str());
	
	printIndent(indent + 1);
	printf("statements:\n");
	if (m_statements) m_statements->printTree(indent + 2);
}

