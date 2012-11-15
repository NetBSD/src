/*
 * File:	ElftosbAST.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_ElftosbAST_h_)
#define _ElftosbAST_h_

#include "stdafx.h"
#include <string>
#include <list>
#include "smart_ptr.h"
#include "EvalContext.h"

namespace elftosb
{

// forward reference
class SymbolASTNode;

/*!
 * \brief Token location in the source file.
 */
struct token_loc_t
{
	int m_firstLine;	//!< Starting line of the token.
	int m_lastLine;		//!< Ending line of the token.
};

/*!
 * \brief The base class for all AST node classes.
 */
class ASTNode
{
public:
	//! \brief Default constructor.
	ASTNode() : m_parent(0) {}
	
	//! \brief Constructor taking a parent node.
	ASTNode(ASTNode * parent) : m_parent(parent) {}
	
	//! \brief Copy constructor.
	ASTNode(const ASTNode & other) : m_parent(other.m_parent) {}
	
	//! \brief Destructor.
	virtual ~ASTNode() {}
	
	//! \brief Returns an exact duplicate of this object.
	virtual ASTNode * clone() const = 0;
	
	//! \brief Returns the name of the object's class.
	virtual std::string nodeName() const { return "ASTNode"; }

	//! \name Parents
	//@{
	virtual ASTNode * getParent() const { return m_parent; }
	virtual void setParent(ASTNode * newParent) { m_parent = newParent; }
	//@}
	
	//! \name Tree printing
	//@{
	virtual void printTree() const { printTree(0); }
	virtual void printTree(int indent) const;
	//@}
	
	//! \name Location
	//@{
	virtual void setLocation(token_loc_t & loc) { m_location = loc; }
	virtual void setLocation(token_loc_t & first, token_loc_t & last);
	virtual void setLocation(ASTNode * loc) { setLocation(loc->getLocation()); }
	virtual void setLocation(ASTNode * first, ASTNode * last);
	
	virtual token_loc_t & getLocation() { return m_location; }
	virtual const token_loc_t & getLocation() const { return m_location; }
	
	virtual int getFirstLine() { return m_location.m_firstLine; }
	virtual int getLastLine() { return m_location.m_lastLine; }
	//@}

protected:
	ASTNode * m_parent;	//!< Pointer to parent node of this object. May be NULL.
	token_loc_t m_location;	//!< Location of this node in the source file.
	
	//! \brief Prints \a indent number of spaces.
	void printIndent(int indent) const;
};

/*!
 * \brief AST node that contains other AST nodes.
 *
 * Unlike other AST nodes, the location of a ListASTNode is computed dynamically
 * based on the nodes in its list. Or mostly dynamic at least. The updateLocation()
 * method is used to force the list object to recalculate its beginning and ending
 * line numbers.
 *
 * \todo Figure out why it crashes in the destructor when the
 *       ast_list_t type is a list of smart_ptr<ASTNode>.
 */
class ListASTNode : public ASTNode
{
public:
	typedef std::list< /*smart_ptr<ASTNode>*/ ASTNode * > ast_list_t;	
	typedef ast_list_t::iterator iterator;
	typedef ast_list_t::const_iterator const_iterator;

public:
	ListASTNode() {}
	
	ListASTNode(const ListASTNode & other);
	
	virtual ~ListASTNode();
	
	virtual ASTNode * clone() const { return new ListASTNode(*this); }
	
	virtual std::string nodeName() const { return "ListASTNode"; }

	virtual void printTree(int indent) const;

	//! \name List operations
	//@{
	//! \brief Adds \a node to the end of the ordered list of child nodes.
	virtual void appendNode(ASTNode * node);
	
	//! \brief Returns the number of nodes in this list.
	virtual unsigned nodeCount() const { return static_cast<unsigned>(m_list.size()); }
	//@}

	//! \name Node iterators
	//@{
	inline iterator begin() { return m_list.begin(); }
	inline iterator end() { return m_list.end(); }

	inline const_iterator begin() const { return m_list.begin(); }
	inline const_iterator end() const { return m_list.end(); }
	//@}
	
	//! \name Location
	//@{
	virtual void updateLocation();
	//@}

protected:
	ast_list_t m_list;	//!< Ordered list of child nodes.
};

/*!
 *
 */
class OptionsBlockASTNode : public ASTNode
{
public:
	OptionsBlockASTNode(ListASTNode * options) : ASTNode(), m_options(options) {}
	
	inline ListASTNode * getOptions() { return m_options; }
	
	virtual ASTNode * clone() const { return NULL; }

protected:
	smart_ptr<ListASTNode> m_options;
};

/*!
 *
 */
class ConstantsBlockASTNode : public ASTNode
{
public:
	ConstantsBlockASTNode(ListASTNode * constants) : ASTNode(), m_constants(constants) {}
	
	inline ListASTNode * getConstants() { return m_constants; }
	
	virtual ASTNode * clone() const { return NULL; }
	
protected:
	smart_ptr<ListASTNode> m_constants;
};

/*!
 *
 */
class SourcesBlockASTNode : public ASTNode
{
public:
	SourcesBlockASTNode(ListASTNode * sources) : ASTNode(), m_sources(sources) {}
	
	inline ListASTNode * getSources() { return m_sources; }
	
	virtual ASTNode * clone() const { return NULL; }
	
protected:
	smart_ptr<ListASTNode> m_sources;
};

/*!
 * \brief Root node for the entire file.
 */
class CommandFileASTNode : public ASTNode
{
public:
	CommandFileASTNode();
	CommandFileASTNode(const CommandFileASTNode & other);
	
	virtual std::string nodeName() const { return "CommandFileASTNode"; }
	
	virtual ASTNode * clone() const { return new CommandFileASTNode(*this); }

	virtual void printTree(int indent) const;

	inline void setBlocks(ListASTNode * blocks) { m_blocks = blocks; }
	inline void setOptions(ListASTNode * options) { m_options = options; }
	inline void setConstants(ListASTNode * constants) { m_constants = constants; }
	inline void setSources(ListASTNode * sources) { m_sources = sources; }
	inline void setSections(ListASTNode * sections) { m_sections = sections; }
	
	inline ListASTNode * getBlocks() { return m_blocks; }
	inline ListASTNode * getOptions() { return m_options; }
	inline ListASTNode * getConstants() { return m_constants; }
	inline ListASTNode * getSources() { return m_sources; }
	inline ListASTNode * getSections() { return m_sections; }

protected:
	smart_ptr<ListASTNode> m_blocks;
	smart_ptr<ListASTNode> m_options;
	smart_ptr<ListASTNode> m_constants;
	smart_ptr<ListASTNode> m_sources;
	smart_ptr<ListASTNode> m_sections;
};

/*!
 * \brief Abstract base class for all expression AST nodes.
 */
class ExprASTNode : public ASTNode
{
public:
	ExprASTNode() : ASTNode() {}
	ExprASTNode(const ExprASTNode & other) : ASTNode(other) {}

	virtual std::string nodeName() const { return "ExprASTNode"; }

	//! \brief Evaluate the expression and produce a result node to replace this one.
	//!
	//! The default implementation simply return this node unmodified. This
	//! method is responsible for deleting any nodes that are no longer needed.
	//! (?) how to delete this?
	virtual ExprASTNode * reduce(EvalContext & context) { return this; }
	
	int_size_t resultIntSize(int_size_t a, int_size_t b);
};

/*!
 *
 */
class IntConstExprASTNode : public ExprASTNode
{
public:
	IntConstExprASTNode(uint32_t value, int_size_t size=kWordSize)
	:	ExprASTNode(), m_value(value), m_size(size)
	{
	}
	
	IntConstExprASTNode(const IntConstExprASTNode & other);

	virtual std::string nodeName() const { return "IntConstExprASTNode"; }
	
	virtual ASTNode * clone() const { return new IntConstExprASTNode(*this); }

	virtual void printTree(int indent) const;
	
	uint32_t getValue() const { return m_value; }
	int_size_t getSize() const { return m_size; }

protected:
	uint32_t m_value;
	int_size_t m_size;
};

/*!
 *
 */
class VariableExprASTNode : public ExprASTNode
{
public:
	VariableExprASTNode(std::string * name) : ExprASTNode(), m_variable(name) {}
	VariableExprASTNode(const VariableExprASTNode & other);
	
	inline std::string * getVariableName() { return m_variable; }
	
	virtual ASTNode * clone() const { return new VariableExprASTNode(*this); }
	
	virtual std::string nodeName() const { return "VariableExprASTNode"; }

	virtual void printTree(int indent) const;
	
	virtual ExprASTNode * reduce(EvalContext & context);
	
protected:
	smart_ptr<std::string> m_variable;
};

/*!
 * \brief Expression node for a symbol reference.
 *
 * The symbol evaluates to its address.
 */
class SymbolRefExprASTNode : public ExprASTNode
{
public:
	SymbolRefExprASTNode(SymbolASTNode * sym) : ExprASTNode(), m_symbol(sym) {}
	SymbolRefExprASTNode(const SymbolRefExprASTNode & other);
	
	virtual ASTNode * clone() const { return new SymbolRefExprASTNode(*this); }
	
	virtual std::string nodeName() const { return "SymbolRefExprASTNode"; }
	
	virtual void printTree(int indent) const;
	
	virtual ExprASTNode * reduce(EvalContext & context);
	
protected:
	SymbolASTNode * m_symbol;
};

/*!
 * \brief Negates an expression.
 */
class NegativeExprASTNode : public ExprASTNode
{
public:
	NegativeExprASTNode(ExprASTNode * expr) : ExprASTNode(), m_expr(expr) {}
	NegativeExprASTNode(const NegativeExprASTNode & other);

	virtual ASTNode * clone() const { return new NegativeExprASTNode(*this); }

	virtual std::string nodeName() const { return "NegativeExprASTNode"; }
	
	virtual void printTree(int indent) const;
	
	virtual ExprASTNode * reduce(EvalContext & context);
	
	ExprASTNode * getExpr() { return m_expr; }

protected:
	smart_ptr<ExprASTNode> m_expr;
};

/*!
 * \brief Performa a boolean inversion.
 */
class BooleanNotExprASTNode : public ExprASTNode
{
public:
	BooleanNotExprASTNode(ExprASTNode * expr) : ExprASTNode(), m_expr(expr) {}
	BooleanNotExprASTNode(const BooleanNotExprASTNode & other);

	virtual ASTNode * clone() const { return new BooleanNotExprASTNode(*this); }

	virtual std::string nodeName() const { return "BooleanNotExprASTNode"; }
	
	virtual void printTree(int indent) const;
	
	virtual ExprASTNode * reduce(EvalContext & context);
	
	ExprASTNode * getExpr() { return m_expr; }

protected:
	smart_ptr<ExprASTNode> m_expr;
};

/*!
 * \brief Calls a built-in function with a source as the parameter.
 */
class SourceFileFunctionASTNode : public ExprASTNode
{
public:
	SourceFileFunctionASTNode(std::string * functionName, std::string * sourceFileName) : ExprASTNode(), m_functionName(functionName), m_sourceFile(sourceFileName) {}
	SourceFileFunctionASTNode(const SourceFileFunctionASTNode & other);

	virtual ASTNode * clone() const { return new SourceFileFunctionASTNode(*this); }

	virtual std::string nodeName() const { return "SourceFileFunctionASTNode"; }
	
	virtual void printTree(int indent) const;
	
	virtual ExprASTNode * reduce(EvalContext & context);
	
	std::string * getFunctionName() { return m_functionName; }
	std::string * getSourceFile() { return m_sourceFile; }

protected:
	smart_ptr<std::string> m_functionName;
	smart_ptr<std::string> m_sourceFile;
};

/*!
 * \brief Returns true or false depending on whether a constant is defined.
 */
class DefinedOperatorASTNode : public ExprASTNode
{
public:
	DefinedOperatorASTNode(std::string * constantName) : ExprASTNode(), m_constantName(constantName) {}
	DefinedOperatorASTNode(const DefinedOperatorASTNode & other);

	virtual ASTNode * clone() const { return new DefinedOperatorASTNode(*this); }

	virtual std::string nodeName() const { return "DefinedOperatorASTNode"; }
	
	virtual void printTree(int indent) const;
	
	virtual ExprASTNode * reduce(EvalContext & context);
	
	std::string * getConstantName() { return m_constantName; }

protected:
	smart_ptr<std::string> m_constantName;	//!< Name of the constant.
};

class SymbolASTNode;

/*!
 * \brief Returns an integer that is the size in bytes of the operand.
 */
class SizeofOperatorASTNode : public ExprASTNode
{
public:
	SizeofOperatorASTNode(std::string * constantName) : ExprASTNode(), m_constantName(constantName), m_symbol() {}
	SizeofOperatorASTNode(SymbolASTNode * symbol) : ExprASTNode(), m_constantName(), m_symbol(symbol) {}
	SizeofOperatorASTNode(const SizeofOperatorASTNode & other);

	virtual ASTNode * clone() const { return new SizeofOperatorASTNode(*this); }

	virtual std::string nodeName() const { return "SizeofOperatorASTNode"; }
	
	virtual void printTree(int indent) const;
	
	virtual ExprASTNode * reduce(EvalContext & context);
	
	std::string * getConstantName() { return m_constantName; }
	SymbolASTNode * getSymbol() { return m_symbol; }

protected:
	smart_ptr<std::string> m_constantName;	//!< Name of the constant.
	smart_ptr<SymbolASTNode> m_symbol;	//!< Symbol reference. If this is non-NULL then the constant name is used instead.
};

/*!
 *
 */
class BinaryOpExprASTNode : public ExprASTNode
{
public:
	enum operator_t
	{
		kAdd,
		kSubtract,
		kMultiply,
		kDivide,
		kModulus,
		kPower,
		kBitwiseAnd,
		kBitwiseOr,
		kBitwiseXor,
		kShiftLeft,
		kShiftRight,
		kLessThan,
		kGreaterThan,
		kLessThanEqual,
		kGreaterThanEqual,
		kEqual,
		kNotEqual,
		kBooleanAnd,
		kBooleanOr
	};
	
	BinaryOpExprASTNode(ExprASTNode * left, operator_t op, ExprASTNode * right)
	:	ExprASTNode(), m_left(left), m_op(op), m_right(right)
	{
	}
	
	BinaryOpExprASTNode(const BinaryOpExprASTNode & other);
	
	virtual ASTNode * clone() const { return new BinaryOpExprASTNode(*this); }

	virtual std::string nodeName() const { return "BinaryOpExprASTNode"; }
	
	virtual void printTree(int indent) const;
	
	virtual ExprASTNode * reduce(EvalContext & context);
	
	ExprASTNode * getLeftExpr() { return m_left; }
	ExprASTNode * getRightExpr() { return m_right; }
	operator_t getOp() const { return m_op; }

protected:
	smart_ptr<ExprASTNode> m_left;
	smart_ptr<ExprASTNode> m_right;
	operator_t m_op;
	
	std::string getOperatorName() const;
};

/*!
 * \brief Negates an expression.
 */
class IntSizeExprASTNode : public ExprASTNode
{
public:
	IntSizeExprASTNode(ExprASTNode * expr, int_size_t intSize) : ExprASTNode(), m_expr(expr), m_size(intSize) {}
	IntSizeExprASTNode(const IntSizeExprASTNode & other);
	
	virtual ASTNode * clone() const { return new IntSizeExprASTNode(*this); }

	virtual std::string nodeName() const { return "IntSizeExprASTNode"; }
	
	virtual void printTree(int indent) const;
	
	virtual ExprASTNode * reduce(EvalContext & context);
	
	ExprASTNode * getExpr() { return m_expr; }
	int_size_t getIntSize() { return m_size; }

protected:
	smart_ptr<ExprASTNode> m_expr;
	int_size_t m_size;
};

/*!
 * Base class for const AST nodes.
 */
class ConstASTNode : public ASTNode
{
public:
	ConstASTNode() : ASTNode() {}
	ConstASTNode(const ConstASTNode & other) : ASTNode(other) {}

protected:
};

/*!
 *
 */
class ExprConstASTNode : public ConstASTNode
{
public:
	ExprConstASTNode(ExprASTNode * expr) : ConstASTNode(), m_expr(expr) {}
	ExprConstASTNode(const ExprConstASTNode & other);
	
	virtual ASTNode * clone() const { return new ExprConstASTNode(*this); }

	virtual std::string nodeName() const { return "ExprConstASTNode"; }
	
	virtual void printTree(int indent) const;
	
	ExprASTNode * getExpr() { return m_expr; }

protected:
	smart_ptr<ExprASTNode> m_expr;
};

/*!
 *
 */
class StringConstASTNode : public ConstASTNode
{
public:
	StringConstASTNode(std::string * value) : ConstASTNode(), m_value(value) {}
	StringConstASTNode(const StringConstASTNode & other);
	
	virtual ASTNode * clone() const { return new StringConstASTNode(*this); }

	virtual std::string nodeName() const { return "StringConstASTNode"; }
	
	virtual void printTree(int indent) const;

	std::string * getString() { return m_value; }

protected:
	smart_ptr<std::string> m_value;
};

/*!
 *
 */
class BlobConstASTNode : public ConstASTNode
{
public:
	BlobConstASTNode(Blob * value) : ConstASTNode(), m_blob(value) {}
	BlobConstASTNode(const BlobConstASTNode & other);
	
	virtual ASTNode * clone() const { return new BlobConstASTNode(*this); }

	virtual std::string nodeName() const { return "BlobConstASTNode"; }
	
	virtual void printTree(int indent) const;

	Blob * getBlob() { return m_blob; }

protected:
	smart_ptr<Blob> m_blob;
};

// Forward declaration.
struct hab_ivt;

/*!
 * \brief Node for a constant IVT structure as used by HAB4.
 */
class IVTConstASTNode : public ConstASTNode
{
public:
    IVTConstASTNode() : ConstASTNode(), m_fields() {}
    IVTConstASTNode(const IVTConstASTNode & other);
    
	virtual ASTNode * clone() const { return new IVTConstASTNode(*this); }
    
	virtual std::string nodeName() const { return "IVTConstASTNode"; }

	virtual void printTree(int indent) const;
    
    void setFieldAssignments(ListASTNode * fields) { m_fields = fields; }
    ListASTNode * getFieldAssignments() { return m_fields; }

protected:
    //! Fields of the IVT are set through assignment statements.
    smart_ptr<ListASTNode> m_fields;
};

/*!
 *
 */
class AssignmentASTNode : public ASTNode
{
public:
	AssignmentASTNode(std::string * ident, ASTNode * value)
	:	ASTNode(), m_ident(ident), m_value(value)
	{
	}
	
	AssignmentASTNode(const AssignmentASTNode & other);
	
	virtual ASTNode * clone() const { return new AssignmentASTNode(*this); }

	virtual std::string nodeName() const { return "AssignmentASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline std::string * getIdent() { return m_ident; }
	inline ASTNode * getValue() { return m_value; }

protected:
	smart_ptr<std::string> m_ident;
	smart_ptr<ASTNode> m_value;
};

/*!
 * Base class for PathSourceDefASTNode and ExternSourceDefASTNode.
 */
class SourceDefASTNode : public ASTNode
{
public:
	SourceDefASTNode(std::string * name) : m_name(name) {}
	SourceDefASTNode(const SourceDefASTNode & other);
	
	inline std::string * getName() { return m_name; }

	inline void setAttributes(ListASTNode * attributes) { m_attributes = attributes; }
	inline ListASTNode * getAttributes() { return m_attributes; }
	
protected:
	smart_ptr<std::string> m_name;
	smart_ptr<ListASTNode> m_attributes;
};

/*!
 *
 */
class PathSourceDefASTNode : public SourceDefASTNode
{
public:
	PathSourceDefASTNode(std::string * name, std::string * path)
	:	SourceDefASTNode(name), m_path(path)
	{
	}
	
	PathSourceDefASTNode(const PathSourceDefASTNode & other);
	
	virtual PathSourceDefASTNode * clone() const { return new PathSourceDefASTNode(*this); }

	virtual std::string nodeName() const { return "PathSourceDefASTNode"; }
	
	virtual void printTree(int indent) const;
	
	std::string * getPath() { return m_path; }
	
protected:
	smart_ptr<std::string> m_path;
};

/*!
 *
 */
class ExternSourceDefASTNode : public SourceDefASTNode
{
public:
	ExternSourceDefASTNode(std::string * name, ExprASTNode * expr)
	:	SourceDefASTNode(name), m_expr(expr)
	{
	}
	
	ExternSourceDefASTNode(const ExternSourceDefASTNode & other);
	
	virtual ASTNode * clone() const { return new ExternSourceDefASTNode(*this); }

	virtual std::string nodeName() const { return "ExternSourceDefASTNode"; }
	
	virtual void printTree(int indent) const;
	
	ExprASTNode * getSourceNumberExpr() { return m_expr; }

protected:
	smart_ptr<ExprASTNode> m_expr;
};

/*!
 *
 */
class SectionContentsASTNode : public ASTNode
{
public:
	SectionContentsASTNode() : m_sectionExpr() {}
	SectionContentsASTNode(ExprASTNode * section) : m_sectionExpr(section) {}
	SectionContentsASTNode(const SectionContentsASTNode & other);
	
	virtual ASTNode * clone() const { return new SectionContentsASTNode(*this); }

	virtual std::string nodeName() const { return "SectionContentsASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline void setSectionNumberExpr(ExprASTNode * section)
	{
		m_sectionExpr = section;
	}
	
	inline ExprASTNode * getSectionNumberExpr()
	{
		return m_sectionExpr;
	}
	
	inline void setOptions(ListASTNode * options)
	{
		m_options = options;
	}
	
	inline ListASTNode * getOptions()
	{
		return m_options;
	}

protected:
	smart_ptr<ExprASTNode> m_sectionExpr;
	smart_ptr<ListASTNode> m_options;
};

/*!
 * @brief Node representing a raw binary section definition.
 */
class DataSectionContentsASTNode : public SectionContentsASTNode
{
public:
	DataSectionContentsASTNode(ASTNode * contents)
	:	SectionContentsASTNode(), m_contents(contents)
	{
	}
	
	DataSectionContentsASTNode(const DataSectionContentsASTNode & other);
	
	virtual ASTNode * clone() const { return new DataSectionContentsASTNode(*this); }

	virtual std::string nodeName() const { return "DataSectionContentsASTNode"; }
	
	virtual void printTree(int indent) const;
	
	ASTNode * getContents() { return m_contents; }
	
protected:
	smart_ptr<ASTNode> m_contents;
};

/*!
 *
 */
class BootableSectionContentsASTNode : public SectionContentsASTNode
{
public:
	BootableSectionContentsASTNode(ListASTNode * statements)
	:	SectionContentsASTNode(), m_statements(statements)
	{
	}
	
	BootableSectionContentsASTNode(const BootableSectionContentsASTNode & other);
	
	virtual ASTNode * clone() const { return new BootableSectionContentsASTNode(*this); }

	virtual std::string nodeName() const { return "BootableSectionContentsASTNode"; }
	
	virtual void printTree(int indent) const;
	
	ListASTNode * getStatements() { return m_statements; }

protected:
	smart_ptr<ListASTNode> m_statements;
};

/*!
 *
 */
class StatementASTNode : public ASTNode
{
public:
	StatementASTNode() : ASTNode() {}
	StatementASTNode(const StatementASTNode & other) : ASTNode(other) {}

protected:
};

/*!
 *
 */
class IfStatementASTNode : public StatementASTNode
{
public:
	IfStatementASTNode() : StatementASTNode(), m_ifStatements(), m_nextIf(), m_elseStatements() {}
	IfStatementASTNode(const IfStatementASTNode & other);
	
	virtual ASTNode * clone() const { return new IfStatementASTNode(*this); }
	
	void setConditionExpr(ExprASTNode * expr) { m_conditionExpr = expr; }
	ExprASTNode * getConditionExpr() { return m_conditionExpr; }
	
	void setIfStatements(ListASTNode * statements) { m_ifStatements = statements; }
	ListASTNode * getIfStatements() { return m_ifStatements; }
	
	void setNextIf(IfStatementASTNode * nextIf) { m_nextIf = nextIf; }
	IfStatementASTNode * getNextIf() { return m_nextIf; }
	
	void setElseStatements(ListASTNode * statements) { m_elseStatements = statements; }
	ListASTNode * getElseStatements() { return m_elseStatements; }

protected:
	smart_ptr<ExprASTNode> m_conditionExpr;	//!< Boolean expression.
	smart_ptr<ListASTNode> m_ifStatements;	//!< List of "if" section statements.
	smart_ptr<IfStatementASTNode> m_nextIf;	//!< Link to next "else if". If this is non-NULL then #m_elseStatements must be NULL and vice-versa.
	smart_ptr<ListASTNode> m_elseStatements;	//!< Statements for the "else" part of the statements.
};

/*!
 * \brief Statement to insert a ROM_MODE_CMD command.
 */
class ModeStatementASTNode : public StatementASTNode
{
public:
	ModeStatementASTNode() : StatementASTNode(), m_modeExpr() {}
	ModeStatementASTNode(ExprASTNode * modeExpr) : StatementASTNode(), m_modeExpr(modeExpr) {}
	ModeStatementASTNode(const ModeStatementASTNode & other);
	
	virtual ASTNode * clone() const { return new ModeStatementASTNode(*this); }
	
	virtual std::string nodeName() const { return "ModeStatementASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline void setModeExpr(ExprASTNode * modeExpr) { m_modeExpr = modeExpr; }
	inline ExprASTNode * getModeExpr() { return m_modeExpr; }

protected:
	smart_ptr<ExprASTNode> m_modeExpr;	//!< Expression that evaluates to the new boot mode.
};

/*!
 * \brief Statement to print a message to the elftosb user.
 */
class MessageStatementASTNode : public StatementASTNode
{
public:
	enum _message_type
	{
		kInfo,	//!< Prints an informational messag to the user.
		kWarning,	//!< Prints a warning to the user.
		kError	//!< Throws an error exception.
	};
	
	typedef enum _message_type message_type_t;
	
public:
	MessageStatementASTNode(message_type_t messageType, std::string * message) : StatementASTNode(), m_type(messageType), m_message(message) {}
	MessageStatementASTNode(const MessageStatementASTNode & other);
	
	virtual ASTNode * clone() const { return new MessageStatementASTNode(*this); }
	
	virtual std::string nodeName() const { return "MessageStatementASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline message_type_t getType() { return m_type; }
	inline std::string * getMessage() { return m_message; }
	
	const char * getTypeName() const;

protected:
	message_type_t m_type;
	smart_ptr<std::string> m_message;	//!< Message to report.
};

/*!
 * \brief AST node for a load statement.
 */
class LoadStatementASTNode : public StatementASTNode
{
public:
	LoadStatementASTNode()
	:	StatementASTNode(), m_data(), m_target(), m_isDCDLoad(false)
	{
	}
	
	LoadStatementASTNode(ASTNode * data, ASTNode * target)
	:	StatementASTNode(), m_data(data), m_target(), m_isDCDLoad(false)
	{
	}
	
	LoadStatementASTNode(const LoadStatementASTNode & other);
	
	virtual ASTNode * clone() const { return new LoadStatementASTNode(*this); }

	virtual std::string nodeName() const { return "LoadStatementASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline void setData(ASTNode * data) { m_data = data; }
	inline ASTNode * getData() { return m_data; }
	
	inline void setTarget(ASTNode * target) { m_target = target; }
	inline ASTNode * getTarget() { return m_target; }
	
	inline void setDCDLoad(bool isDCD) { m_isDCDLoad = isDCD; }
	inline bool isDCDLoad() const { return m_isDCDLoad; }
	
protected:
	smart_ptr<ASTNode> m_data;
	smart_ptr<ASTNode> m_target;
	bool m_isDCDLoad;
};

/*!
 * \brief AST node for a call statement.
 */
class CallStatementASTNode : public StatementASTNode
{
public:
	//! Possible sub-types of call statements.
	typedef enum {
		kCallType,
		kJumpType
	} call_type_t;
	
public:
	CallStatementASTNode(call_type_t callType=kCallType)
	:	StatementASTNode(), m_type(callType), m_target(), m_arg(), m_isHAB(false)
	{
	}
	
	CallStatementASTNode(call_type_t callType, ASTNode * target, ASTNode * arg)
	:	StatementASTNode(), m_type(callType), m_target(target), m_arg(arg), m_isHAB(false)
	{
	}
	
	CallStatementASTNode(const CallStatementASTNode & other);
	
	virtual ASTNode * clone() const { return new CallStatementASTNode(*this); }

	virtual std::string nodeName() const { return "CallStatementASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline void setCallType(call_type_t callType) { m_type = callType; }
	inline call_type_t getCallType() { return m_type; }
	
	inline void setTarget(ASTNode * target) { m_target = target; }
	inline ASTNode * getTarget() { return m_target; }
	
	inline void setArgument(ASTNode * arg) { m_arg = arg; }
	inline ASTNode * getArgument() { return m_arg; }
	
	inline void setIsHAB(bool isHAB) { m_isHAB = isHAB; }
	inline bool isHAB() const { return m_isHAB; }
	
protected:
	call_type_t m_type;
	smart_ptr<ASTNode> m_target;	//!< This becomes the IVT address in HAB mode.
	smart_ptr<ASTNode> m_arg;
	bool m_isHAB;
};

/*!
 *
 */
class SourceASTNode : public ASTNode
{
public:
	SourceASTNode(std::string * name) : ASTNode(), m_name(name) {}
	SourceASTNode(const SourceASTNode & other);
	
	virtual ASTNode * clone() const { return new SourceASTNode(*this); }

	virtual std::string nodeName() const { return "SourceASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline std::string * getSourceName() { return m_name; }
	
protected:
	smart_ptr<std::string> m_name;
};

/*!
 * \brief List of section matches for a particular source name.
 */
class SectionMatchListASTNode : public ASTNode
{
public:
	SectionMatchListASTNode(ListASTNode * sections)
	:	ASTNode(), m_sections(sections), m_source()
	{
	}
	
	SectionMatchListASTNode(ListASTNode * sections, std::string * source)
	:	ASTNode(), m_sections(sections), m_source(source)
	{
	}
	
	SectionMatchListASTNode(const SectionMatchListASTNode & other);
	
	virtual ASTNode * clone() const { return new SectionMatchListASTNode(*this); }

	virtual std::string nodeName() const { return "SectionMatchListASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline ListASTNode * getSections() { return m_sections; }
	inline std::string * getSourceName() { return m_source; }
	
protected:
	smart_ptr<ListASTNode> m_sections;
	smart_ptr<std::string> m_source;
};

/*!
 * \brief AST node for a section glob.
 *
 * Can be assigned an include or exclude action for when this node is part of a
 * SectionMatchListASTNode.
 */
class SectionASTNode : public ASTNode
{
public:
	//! Possible actions for a section match list.
	typedef enum
	{
		kInclude,	//!< Include sections matched by this node.
		kExclude	//!< Exclude sections matched by this node.
	} match_action_t;
	
public:
	SectionASTNode(std::string * name)
	:	ASTNode(), m_action(kInclude), m_name(name), m_source()
	{
	}
	
	SectionASTNode(std::string * name, match_action_t action)
	:	ASTNode(), m_action(action), m_name(name), m_source()
	{
	}
	
	SectionASTNode(std::string * name, std::string * source)
	:	ASTNode(), m_action(kInclude), m_name(name), m_source(source)
	{
	}
	
	SectionASTNode(const SectionASTNode & other);
	
	virtual ASTNode * clone() const { return new SectionASTNode(*this); }

	virtual std::string nodeName() const { return "SectionASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline match_action_t getAction() { return m_action; }
	inline std::string * getSectionName() { return m_name; }
	inline std::string * getSourceName() { return m_source; }
	
protected:
	match_action_t m_action;
	smart_ptr<std::string> m_name;
	smart_ptr<std::string> m_source;
};

/*!
 *
 */
class SymbolASTNode : public ASTNode
{
public:
	SymbolASTNode()
	:	ASTNode(), m_symbol(), m_source()
	{
	}

	SymbolASTNode(std::string * symbol, std::string * source=0)
	:	ASTNode(), m_symbol(symbol), m_source(source)
	{
	}
	
	SymbolASTNode(const SymbolASTNode & other);
	
	virtual ASTNode * clone() const { return new SymbolASTNode(*this); }

	virtual std::string nodeName() const { return "SymbolASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline void setSymbolName(std::string * symbol) { m_symbol = symbol; }
	inline std::string * getSymbolName() { return m_symbol; }
	
	inline void setSource(std::string * source) { m_source = source; }
	inline std::string * getSource() { return m_source; }

protected:
	smart_ptr<std::string> m_symbol;	//!< Required.
	smart_ptr<std::string> m_source;	//!< Optional, may be NULL;
};

/*!
 * If the end of the range is NULL, then only a single address was specified.
 */
class AddressRangeASTNode : public ASTNode
{
public:
	AddressRangeASTNode()
	:	ASTNode(), m_begin(), m_end()
	{
	}
	
	AddressRangeASTNode(ASTNode * begin, ASTNode * end)
	:	ASTNode(), m_begin(begin), m_end(end)
	{
	}
	
	AddressRangeASTNode(const AddressRangeASTNode & other);
	
	virtual ASTNode * clone() const { return new AddressRangeASTNode(*this); }

	virtual std::string nodeName() const { return "AddressRangeASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline void setBegin(ASTNode * begin) { m_begin = begin; }
	inline ASTNode * getBegin() { return m_begin; }
	
	inline void setEnd(ASTNode * end) { m_end = end; }
	inline ASTNode * getEnd() { return m_end; }
	
protected:
	smart_ptr<ASTNode> m_begin;
	smart_ptr<ASTNode> m_end;
};

/*!
 *
 */
class NaturalLocationASTNode : public ASTNode
{
public:
	NaturalLocationASTNode()
	:	ASTNode()
	{
	}
	
	NaturalLocationASTNode(const NaturalLocationASTNode & other)
	:	ASTNode(other)
	{
	}
	
	virtual ASTNode * clone() const { return new NaturalLocationASTNode(*this); }

	virtual std::string nodeName() const { return "NaturalLocationASTNode"; }
};

/*!
 *
 */
class FromStatementASTNode : public StatementASTNode
{
public:
	FromStatementASTNode() : StatementASTNode() {}
	FromStatementASTNode(std::string * source, ListASTNode * statements);
	FromStatementASTNode(const FromStatementASTNode & other);
	
	virtual ASTNode * clone() const { return new FromStatementASTNode(*this); }

	virtual std::string nodeName() const { return "FromStatementASTNode"; }
	
	virtual void printTree(int indent) const;
	
	inline void setSourceName(std::string * source) { m_source = source; }
	inline std::string * getSourceName() { return m_source; }
	
	inline void setStatements(ListASTNode * statements) { m_statements = statements; }
	inline ListASTNode * getStatements() { return m_statements; }

protected:
	smart_ptr<std::string> m_source;
	smart_ptr<ListASTNode> m_statements;
};

}; // namespace elftosb

#endif // _ElftosbAST_h_
