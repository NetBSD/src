/*
 * File:	ConversionController.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "ConversionController.h"
#include <stdexcept>
#include "EvalContext.h"
#include "ElftosbErrors.h"
#include "GlobMatcher.h"
#include "ExcludesListMatcher.h"
#include "BootImageGenerator.h"
#include "EncoreBootImageGenerator.h"
#include "Logging.h"
#include "OptionDictionary.h"
#include "format_string.h"
#include "SearchPath.h"
#include "DataSourceImager.h"
#include "IVTDataSource.h"
#include <algorithm>

//! Set to 1 to cause the ConversionController to print information about
//! the values that it processes (options, constants, etc.).
#define PRINT_VALUES 1

using namespace elftosb;

// Define the parser function prototype;
extern int yyparse(ElftosbLexer * lexer, CommandFileASTNode ** resultAST);

bool elftosb::g_enableHABSupport = false;

ConversionController::ConversionController()
:	OptionDictionary(),
	m_commandFilePath(),
	m_ast(),
	m_defaultSource(0)
{
	m_context.setSourceFileManager(this);
}

ConversionController::~ConversionController()
{
	// clean up sources
	source_map_t::iterator it = m_sources.begin();
	for (; it != m_sources.end(); ++it)
	{
		if (it->second)
		{
			delete it->second;
		}
	}
}

void ConversionController::setCommandFilePath(const std::string & path)
{
	m_commandFilePath = new std::string(path);
}

//! The paths provided to this method are added to an array and accessed with the
//! "extern(N)" notation in the command file. So the path provided in the third
//! call to addExternalFilePath() will be found with N=2 in the source definition.
void ConversionController::addExternalFilePath(const std::string & path)
{
	m_externPaths.push_back(path);
}

bool ConversionController::hasSourceFile(const std::string & name)
{
	return m_sources.find(name) != m_sources.end();
}

SourceFile * ConversionController::getSourceFile(const std::string & name)
{
	if (!hasSourceFile(name))
	{
		return NULL;
	}
	
	return m_sources[name];
}

SourceFile * ConversionController::getDefaultSourceFile()
{
	return m_defaultSource;
}

//! These steps are executed while running this method:
//!		- The command file is parsed into an abstract syntax tree.
//!		- The list of options is extracted.
//!		- Constant expressions are evaluated.
//!		- The list of source files is extracted and source file objects created.
//!		- Section definitions are extracted.
//!
//! This method does not produce any output. It processes the input files and
//! builds a representation of the output in memory. Use the generateOutput() method
//! to produce a BootImage object after this method returns.
//!
//! \note This method is \e not reentrant. And in fact, the whole class is not designed
//!		to be reentrant.
//!
//! \exception std::runtime_error Any number of problems will cause this exception to
//!		be thrown.
//!
//! \see parseCommandFile()
//! \see processOptions()
//! \see processConstants()
//! \see processSources()
//! \see processSections()
void ConversionController::run()
{
#if PRINT_VALUES
	Log::SetOutputLevel debugLevel(Logger::DEBUG2);
#endif

	parseCommandFile();
	assert(m_ast);
	
	ListASTNode * blocks = m_ast->getBlocks();
	if (!blocks)
	{
		throw std::runtime_error("command file has no blocks");
	}
	
	ListASTNode::iterator it = blocks->begin();
	for (; it != blocks->end(); ++it)
	{
		ASTNode * node = *it;
		
		// Handle an options block.
		OptionsBlockASTNode * options = dynamic_cast<OptionsBlockASTNode *>(node);
		if (options)
		{
			processOptions(options->getOptions());
			continue;
		}
		
		// Handle a constants block.
		ConstantsBlockASTNode * constants = dynamic_cast<ConstantsBlockASTNode *>(node);
		if (constants)
		{
			processConstants(constants->getConstants());
			continue;
		}
		
		// Handle a sources block.
		SourcesBlockASTNode * sources = dynamic_cast<SourcesBlockASTNode *>(node);
		if (sources)
		{
			processSources(sources->getSources());
		}
	}
	
	processSections(m_ast->getSections());
}

//! Opens the command file and runs it through the lexer and parser. The resulting
//! abstract syntax tree is held in the m_ast member variable. After parsing, the
//! command file is closed.
//!
//! \exception std::runtime_error Several problems will cause this exception to be
//!		raised, including an unspecified command file path or an error opening the
//!		file.
void ConversionController::parseCommandFile()
{
	if (!m_commandFilePath)
	{
		throw std::runtime_error("no command file path was provided");
	}
	
	// Search for command file
	std::string actualPath;
	bool found = PathSearcher::getGlobalSearcher().search(*m_commandFilePath, PathSearcher::kFindFile, true, actualPath);
	if (!found)
	{
		throw runtime_error(format_string("unable to find command file %s\n", m_commandFilePath->c_str()));
	}

	// open command file
	std::ifstream commandFile(actualPath.c_str(), ios_base::in | ios_base::binary);
	if (!commandFile.is_open())
	{
		throw std::runtime_error("could not open command file");
	}
	
	try
	{
		// create lexer instance
		ElftosbLexer lexer(commandFile);
//		testLexer(lexer);
		
		CommandFileASTNode * ast = NULL;
		int result = yyparse(&lexer, &ast);
		m_ast = ast;
		
		// check results
		if (result || !m_ast)
		{
			throw std::runtime_error("failed to parse command file");
		}
		
		// dump AST
//		m_ast->printTree(0);
		
		// close command file
		commandFile.close();
	}
	catch (...)
	{
		// close command file
		commandFile.close();
		
		// rethrow exception
		throw;
	}
}

//! Iterates over the option definition AST nodes. elftosb::Value objects are created for
//! each option value and added to the option dictionary.
//!
//! \exception std::runtime_error Various errors will cause this exception to be thrown. These
//!		include AST nodes being an unexpected type or expression not evaluating to integers.
void ConversionController::processOptions(ListASTNode * options)
{
	if (!options)
	{
		return;
	}
	
	ListASTNode::iterator it = options->begin();
	for (; it != options->end(); ++it)
	{
		std::string ident;
		Value * value = convertAssignmentNodeToValue(*it, ident);
		
		// check if this option has already been set
		if (hasOption(ident))
		{
			throw semantic_error(format_string("line %d: option already set", (*it)->getFirstLine()));
		}
		
		// now save the option value in our map
		if (value)
		{
			setOption(ident, value);
		}
	}
}

//! Scans the constant definition AST nodes, evaluates expression nodes by calling their
//! elftosb::ExprASTNode::reduce() method, and updates the evaluation context member so
//! those constant values can be used in other expressions.
//!
//! \exception std::runtime_error Various errors will cause this exception to be thrown. These
//!		include AST nodes being an unexpected type or expression not evaluating to integers.
void ConversionController::processConstants(ListASTNode * constants)
{
	if (!constants)
	{
		return;
	}
	
	ListASTNode::iterator it = constants->begin();
	for (; it != constants->end(); ++it)
	{
		std::string ident;
		Value * value = convertAssignmentNodeToValue(*it, ident);
		
		SizedIntegerValue * intValue = dynamic_cast<SizedIntegerValue*>(value);
		if (!intValue)
		{
			throw semantic_error(format_string("line %d: constant value is an invalid type", (*it)->getFirstLine()));
		}
				
//#if PRINT_VALUES
//		Log::log("constant ");
//		printIntConstExpr(ident, intValue);
//#endif
		
		// record this constant's value in the evaluation context
		m_context.setVariable(ident, intValue->getValue(), intValue->getWordSize());
	}
}

//! \exception std::runtime_error Various errors will cause this exception to be thrown. These
//!		include AST nodes being an unexpected type or expression not evaluating to integers.
//!
//! \todo Handle freeing of dict if an exception occurs.
void ConversionController::processSources(ListASTNode * sources)
{
	if (!sources)
	{
		return;
	}
	
	ListASTNode::iterator it = sources->begin();
	for (; it != sources->end(); ++it)
	{
		SourceDefASTNode * node = dynamic_cast<SourceDefASTNode*>(*it);
		if (!node)
		{
			throw semantic_error(format_string("line %d: source definition node is an unexpected type", node->getFirstLine()));
		}
		
		// get source name and check if it has already been defined
		std::string * name = node->getName();
		if (m_sources.find(*name) != m_sources.end())
		{
			// can't define a source multiple times
			throw semantic_error(format_string("line %d: source already defined", node->getFirstLine()));
		}
		
		// convert attributes into an option dict
		OptionDictionary * dict = new OptionDictionary(this);
		ListASTNode * attrsNode = node->getAttributes();
		if (attrsNode)
		{
			ListASTNode::iterator attrIt = attrsNode->begin();
			for (; attrIt != attrsNode->end(); ++attrIt)
			{
				std::string ident;
				Value * value = convertAssignmentNodeToValue(*attrIt, ident);
				dict->setOption(ident, value);
			}
		}
		
		// figure out which type of source definition this is
		PathSourceDefASTNode * pathNode = dynamic_cast<PathSourceDefASTNode*>(node);
		ExternSourceDefASTNode * externNode = dynamic_cast<ExternSourceDefASTNode*>(node);
		SourceFile * file = NULL;
		
		if (pathNode)
		{
			// explicit path
			std::string * path = pathNode->getPath();
			
#if PRINT_VALUES
			Log::log("source %s => path(%s)\n", name->c_str(), path->c_str());
#endif
			
			try
			{
				file = SourceFile::openFile(*path);
			}
			catch (...)
			{
				// file doesn't exist
				Log::log(Logger::INFO2, "failed to open source file: %s (ignoring for now)\n", path->c_str());
				m_failedSources.push_back(*name);
			}
		}
		else if (externNode)
		{
			// externally provided path
			ExprASTNode * expr = externNode->getSourceNumberExpr()->reduce(m_context);
			IntConstExprASTNode * intConst = dynamic_cast<IntConstExprASTNode*>(expr);
			if (!intConst)
			{
				throw semantic_error(format_string("line %d: expression didn't evaluate to an integer", expr->getFirstLine()));
			}
			
			uint32_t externalFileNumber = static_cast<uint32_t>(intConst->getValue());
			
			// make sure the extern number is valid
			if (externalFileNumber >= 0 && externalFileNumber < m_externPaths.size())
			{
			
#if PRINT_VALUES
			Log::log("source %s => extern(%d=%s)\n", name->c_str(), externalFileNumber, m_externPaths[externalFileNumber].c_str());
#endif
			
				try
				{
					file = SourceFile::openFile(m_externPaths[externalFileNumber]);
				}
				catch (...)
				{
					Log::log(Logger::INFO2, "failed to open source file: %s (ignoring for now)\n", m_externPaths[externalFileNumber].c_str());
					m_failedSources.push_back(*name);
				}
			}
		}
		else
		{
			throw semantic_error(format_string("line %d: unexpected source definition node type", node->getFirstLine()));
		}
		
		if (file)
		{
			// set options
			file->setOptions(dict);
			
			// stick the file object in the source map
			m_sources[*name] = file;
		}
	}
}

void ConversionController::processSections(ListASTNode * sections)
{
	if (!sections)
	{
		Log::log(Logger::WARNING, "warning: no sections were defined in command file");
		return;
	}
	
	ListASTNode::iterator it = sections->begin();
	for (; it != sections->end(); ++it)
	{
		SectionContentsASTNode * node = dynamic_cast<SectionContentsASTNode*>(*it);
		if (!node)
		{
			throw semantic_error(format_string("line %d: section definition is unexpected type", node->getFirstLine()));
		}
		
		// evaluate section number
		ExprASTNode * idExpr = node->getSectionNumberExpr()->reduce(m_context);
		IntConstExprASTNode * idConst = dynamic_cast<IntConstExprASTNode*>(idExpr);
		if (!idConst)
		{
			throw semantic_error(format_string("line %d: section number did not evaluate to an integer", idExpr->getFirstLine()));
		}
		uint32_t sectionID = idConst->getValue();
		
		// Create options context for this section. The options context has the
		// conversion controller as its parent context so it will inherit global options.
		// The context will be set in the section after the section is created below.
		OptionDictionary * optionsDict = new OptionDictionary(this);
		ListASTNode * attrsNode = node->getOptions();
		if (attrsNode)
		{
			ListASTNode::iterator attrIt = attrsNode->begin();
			for (; attrIt != attrsNode->end(); ++attrIt)
			{
				std::string ident;
				Value * value = convertAssignmentNodeToValue(*attrIt, ident);
				optionsDict->setOption(ident, value);
			}
		}
		
		// Now create the actual section object based on its type.
		OutputSection * outputSection = NULL;
		BootableSectionContentsASTNode * bootableSection;
		DataSectionContentsASTNode * dataSection;
		if (bootableSection = dynamic_cast<BootableSectionContentsASTNode*>(node))
		{		
			// process statements into a sequence of operations
			ListASTNode * statements = bootableSection->getStatements();
			OperationSequence * sequence = convertStatementList(statements);

#if 0
			Log::log("section ID = %d\n", sectionID);
			statements->printTree(0);
			
			Log::log("sequence has %d operations\n", sequence->getCount());
			OperationSequence::iterator_t it = sequence->begin();
			for (; it != sequence->end(); ++it)
			{
				Operation * op = *it;
				Log::log("op = %p\n", op);
			}
#endif
			
			// create the output section and add it to the list
			OperationSequenceSection * opSection = new OperationSequenceSection(sectionID);
			opSection->setOptions(optionsDict);
			opSection->getSequence() += sequence;
			outputSection = opSection;
		}
		else if (dataSection = dynamic_cast<DataSectionContentsASTNode*>(node))
		{
			outputSection = convertDataSection(dataSection, sectionID, optionsDict);
		}
		else
		{
			throw semantic_error(format_string("line %d: unexpected section contents type", node->getFirstLine()));
		}
		
		if (outputSection)
		{
			m_outputSections.push_back(outputSection);
		}
	}
}

//! Creates an instance of BinaryDataSection from the AST node passed in the
//! \a dataSection parameter. The section-specific options for this node will
//! have already been converted into an OptionDictionary, the one passed in
//! the \a optionsDict parameter.
//!
//! The \a dataSection node will have as its contents one of the AST node
//! classes that represents a source of data. The member function
//! createSourceFromNode() is used to convert this AST node into an
//! instance of a DataSource subclass. Then the method imageDataSource()
//! converts the segments of the DataSource into a raw binary buffer that
//! becomes the contents of the BinaryDataSection this is returned.
//!
//! \param dataSection The AST node for the data section.
//! \param sectionID Unique tag value the user has assigned to this section.
//! \param optionsDict Options that apply only to this section. This dictionary
//!		will be assigned as the options dictionary for the resulting section
//!		object. Its parent is the conversion controller itself.
//! \return An instance of BinaryDataSection. Its contents are a contiguous
//!		binary representation of the contents of \a dataSection.
OutputSection * ConversionController::convertDataSection(DataSectionContentsASTNode * dataSection, uint32_t sectionID, OptionDictionary * optionsDict)
{
	// Create a data source from the section contents AST node.
	ASTNode * contents = dataSection->getContents();
	DataSource * dataSource = createSourceFromNode(contents);
	
	// Convert the data source to a raw buffer.
	DataSourceImager imager;
	imager.addDataSource(dataSource);
	
	// Then make a data section from the buffer.
	BinaryDataSection * resultSection = new BinaryDataSection(sectionID);
	resultSection->setOptions(optionsDict);
	if (imager.getLength())
	{
		resultSection->setData(imager.getData(), imager.getLength());
	}
	
	return resultSection;
}

//! @param node The AST node instance for the assignment expression.
//! @param[out] ident Upon exit this string will be set the the left hand side of the
//!		assignment expression, the identifier name.
//!
//! @return An object that is a subclass of Value is returned. The specific subclass will
//!		depend on the type of the right hand side of the assignment expression whose AST
//!		node was provided in the @a node argument.
//!
//! @exception semantic_error Thrown for any error where an AST node is an unexpected type.
//!		This may be the @a node argument itself, if it is not an AssignmentASTNode. Or it
//!		may be an unexpected type for either the right or left hand side of the assignment.
//!		The message for the exception will contain a description of the error.
Value * ConversionController::convertAssignmentNodeToValue(ASTNode * node, std::string & ident)
{
	Value * resultValue = NULL;
	
	// each item of the options list should be an assignment node
	AssignmentASTNode * assignmentNode = dynamic_cast<AssignmentASTNode*>(node);
	if (!node)
	{
		throw semantic_error(format_string("line %d: node is wrong type", assignmentNode->getFirstLine()));
	}
	
	// save the left hand side (the identifier) into ident
	ident = *assignmentNode->getIdent();
	
	// get the right hand side and convert it to a Value instance
	ASTNode * valueNode = assignmentNode->getValue();
	StringConstASTNode * str;
	ExprASTNode * expr;
	if (str = dynamic_cast<StringConstASTNode*>(valueNode))
	{
		// the option value is a string constant
		resultValue = new StringValue(str->getString());

//#if PRINT_VALUES
//		Log::log("option %s => \'%s\'\n", ident->c_str(), str->getString()->c_str());
//#endif
	}
	else if (expr = dynamic_cast<ExprASTNode*>(valueNode))
	{
		ExprASTNode * reducedExpr = expr->reduce(m_context);
		IntConstExprASTNode * intConst = dynamic_cast<IntConstExprASTNode*>(reducedExpr);
		if (!intConst)
		{
			throw semantic_error(format_string("line %d: expression didn't evaluate to an integer", expr->getFirstLine()));
		}
		
//#if PRINT_VALUES
//		Log::log("option ");
//		printIntConstExpr(*ident, intConst);
//#endif
		
		resultValue = new SizedIntegerValue(intConst->getValue(), intConst->getSize());
	}
	else
	{
		throw semantic_error(format_string("line %d: right hand side node is an unexpected type", valueNode->getFirstLine()));
	}
	
	return resultValue;
}

//! Builds up a sequence of Operation objects that are equivalent to the
//! statements in the \a statements list. The statement list is simply iterated
//! over and the results of convertOneStatement() are used to build up
//! the final result sequence.
//!
//! \see convertOneStatement()
OperationSequence * ConversionController::convertStatementList(ListASTNode * statements)
{
	OperationSequence * resultSequence = new OperationSequence();
	ListASTNode::iterator it = statements->begin();
	for (; it != statements->end(); ++it)
	{
		StatementASTNode * statement = dynamic_cast<StatementASTNode*>(*it);
		if (!statement)
		{
			throw semantic_error(format_string("line %d: statement node is unexpected type", (*it)->getFirstLine()));
		}
		
		// convert this statement and append it to the result
		OperationSequence * sequence = convertOneStatement(statement);
		if (sequence)
		{
			*resultSequence += sequence;
		}
	}
	
	return resultSequence;
}

//! Uses C++ RTTI to identify the particular subclass of StatementASTNode that
//! the \a statement argument matches. Then the appropriate conversion method
//! is called.
//!
//! \see convertLoadStatement()
//! \see convertCallStatement()
//! \see convertFromStatement()
OperationSequence * ConversionController::convertOneStatement(StatementASTNode * statement)
{
	// see if it's a load statement
	LoadStatementASTNode * load = dynamic_cast<LoadStatementASTNode*>(statement);
	if (load)
	{
		return convertLoadStatement(load);
	}
	
	// see if it's a call statement
	CallStatementASTNode * call = dynamic_cast<CallStatementASTNode*>(statement);
	if (call)
	{
		return convertCallStatement(call);
	}
	
	// see if it's a from statement
	FromStatementASTNode * from = dynamic_cast<FromStatementASTNode*>(statement);
	if (from)
	{
		return convertFromStatement(from);
	}
	
	// see if it's a mode statement
	ModeStatementASTNode * mode = dynamic_cast<ModeStatementASTNode*>(statement);
	if (mode)
	{
		return convertModeStatement(mode);
	}
	
	// see if it's an if statement
	IfStatementASTNode * ifStmt = dynamic_cast<IfStatementASTNode*>(statement);
	if (ifStmt)
	{
		return convertIfStatement(ifStmt);
	}
	
	// see if it's a message statement
	MessageStatementASTNode * messageStmt = dynamic_cast<MessageStatementASTNode*>(statement);
	if (messageStmt)
	{
		// Message statements don't produce operation sequences.
		handleMessageStatement(messageStmt);
		return NULL;
	}
	
	// didn't match any of the expected statement types
	throw semantic_error(format_string("line %d: unexpected statement type", statement->getFirstLine()));
	return NULL;
}

//! Possible load data node types:
//! - StringConstASTNode
//! - ExprASTNode
//! - SourceASTNode
//! - SectionMatchListASTNode
//!
//! Possible load target node types:
//! - SymbolASTNode
//! - NaturalLocationASTNode
//! - AddressRangeASTNode
OperationSequence * ConversionController::convertLoadStatement(LoadStatementASTNode * statement)
{
	LoadOperation * op = NULL;
	
	try
	{
		// build load operation from source and target
		op = new LoadOperation();
		op->setSource(createSourceFromNode(statement->getData()));
		op->setTarget(createTargetFromNode(statement->getTarget()));
		op->setDCDLoad(statement->isDCDLoad());
		
		return new OperationSequence(op);
	}
	catch (...)
	{
		if (op)
		{
			delete op;
		}
		throw;
	}
}

//! Possible call target node types:
//! - SymbolASTNode
//! - ExprASTNode
//!
//! Possible call argument node types:
//! - ExprASTNode
//! - NULL
OperationSequence * ConversionController::convertCallStatement(CallStatementASTNode * statement)
{
	ExecuteOperation * op = NULL;
	
	try
	{
		// create operation from AST nodes
		op = new ExecuteOperation();
		
		bool isHAB = statement->isHAB();
		
		op->setTarget(createTargetFromNode(statement->getTarget()));
		
		// set argument value, which defaults to 0 if no expression was provided
		uint32_t arg = 0;
		ASTNode * argNode = statement->getArgument();
		if (argNode)
		{
			ExprASTNode * argExprNode = dynamic_cast<ExprASTNode*>(argNode);
			if (!argExprNode)
			{
				throw semantic_error(format_string("line %d: call argument is unexpected type", argNode->getFirstLine()));
			}
			argExprNode = argExprNode->reduce(m_context);
			IntConstExprASTNode * intNode = dynamic_cast<IntConstExprASTNode*>(argExprNode);
			if (!intNode)
			{
				throw semantic_error(format_string("line %d: call argument did not evaluate to an integer", argExprNode->getFirstLine()));
			}
			
			arg = intNode->getValue();
		}
		op->setArgument(arg);
		
		// set call type
		switch (statement->getCallType())
		{
			case CallStatementASTNode::kCallType:
				op->setExecuteType(ExecuteOperation::kCall);
				break;
			case CallStatementASTNode::kJumpType:
				op->setExecuteType(ExecuteOperation::kJump);
				break;
		}
		
		// Set the HAB mode flag.
		op->setIsHAB(isHAB);
		
		return new OperationSequence(op);
	}
	catch (...)
	{
		// delete op and rethrow exception
		if (op)
		{
			delete op;
		}
		throw;
	}
}

//! First this method sets the default source to the source identified in
//! the from statement. Then the statements within the from block are
//! processed recursively by calling convertStatementList(). The resulting
//! operation sequence is returned.
OperationSequence * ConversionController::convertFromStatement(FromStatementASTNode * statement)
{
	if (m_defaultSource)
	{
		throw semantic_error(format_string("line %d: from statements cannot be nested", statement->getFirstLine()));
	}
	
	// look up source file instance
	std::string * fromSourceName = statement->getSourceName();
	assert(fromSourceName);
	
	// make sure it's a valid source name
	source_map_t::iterator sourceIt = m_sources.find(*fromSourceName);
	if (sourceIt == m_sources.end())
	{
		throw semantic_error(format_string("line %d: bad source name", statement->getFirstLine()));
	}
	
	// set default source
	m_defaultSource = sourceIt->second;
	assert(m_defaultSource);
	
	// get statements inside the from block
	ListASTNode * fromStatements = statement->getStatements();
	assert(fromStatements);
	
	// produce resulting operation sequence
	OperationSequence * result = convertStatementList(fromStatements);
	
	// restore default source to NULL
	m_defaultSource = NULL;
	
	return result;
}

//! Evaluates the expression to get the new boot mode value. Then creates a
//! BootModeOperation object and returns an OperationSequence containing it.
//!
//! \exception elftosb::semantic_error Thrown if a semantic problem is found with
//!		the boot mode expression.
OperationSequence * ConversionController::convertModeStatement(ModeStatementASTNode * statement)
{
	BootModeOperation * op = NULL;
	
	try
	{
		op = new BootModeOperation();
		
		// evaluate the boot mode expression
		ExprASTNode * modeExprNode = statement->getModeExpr();
		if (!modeExprNode)
		{
			throw semantic_error(format_string("line %d: mode statement has invalid boot mode expression", statement->getFirstLine()));
		}
		modeExprNode = modeExprNode->reduce(m_context);
		IntConstExprASTNode * intNode = dynamic_cast<IntConstExprASTNode*>(modeExprNode);
		if (!intNode)
		{
			throw semantic_error(format_string("line %d: boot mode did not evaluate to an integer", statement->getFirstLine()));
		}
		
		op->setBootMode(intNode->getValue());
		
		return new OperationSequence(op);
	}
	catch (...)
	{
		if (op)
		{
			delete op;
		}
		
		// rethrow exception
		throw;
	}
}

//! Else branches, including else-if, are handled recursively, so there is a limit
//! on the number of them based on the stack size.
//!
//! \return Returns the operation sequence for the branch of the if statement that
//!		evaluated to true. If the statement did not have an else branch and the
//!		condition expression evaluated to false, then NULL will be returned.
//!
//! \todo Handle else branches without recursion.
OperationSequence * ConversionController::convertIfStatement(IfStatementASTNode * statement)
{
	// Get the if's conditional expression.
	ExprASTNode * conditionalExpr = statement->getConditionExpr();
	if (!conditionalExpr)
	{
		throw semantic_error(format_string("line %d: missing or invalid conditional expression", statement->getFirstLine()));
	}
	
	// Reduce the conditional to a single integer.
	conditionalExpr = conditionalExpr->reduce(m_context);
	IntConstExprASTNode * intNode = dynamic_cast<IntConstExprASTNode*>(conditionalExpr);
	if (!intNode)
	{
		throw semantic_error(format_string("line %d: if statement conditional expression did not evaluate to an integer", statement->getFirstLine()));
	}
	
	// Decide which statements to further process by the conditional's boolean value.
	if (intNode->getValue() && statement->getIfStatements())
	{
		return convertStatementList(statement->getIfStatements());
	}
	else if (statement->getElseStatements())
	{
		return convertStatementList(statement->getElseStatements());
	}
	else
	{
		// No else branch and the conditional was false, so there are no operations to return.
		return NULL;
	}
}

//! Message statements are executed immediately, by this method. They are
//! not converted into an abstract operation. All messages are passed through
//! substituteVariables() before being output.
//!
//! \param statement The message statement AST node object.
void ConversionController::handleMessageStatement(MessageStatementASTNode * statement)
{
	string * message = statement->getMessage();
	if (!message)
	{
		throw runtime_error("message statement had no message");
	}
	
	smart_ptr<string> finalMessage = substituteVariables(message);
	
	switch (statement->getType())
	{
		case MessageStatementASTNode::kInfo:
			Log::log(Logger::INFO, "%s\n", finalMessage->c_str());
			break;
		
		case MessageStatementASTNode::kWarning:
			Log::log(Logger::WARNING, "warning: %s\n", finalMessage->c_str());
			break;
		
		case MessageStatementASTNode::kError:
			throw runtime_error(*finalMessage);
			break;
	}
}

//! Performs shell-like variable substitution on the string passed into it.
//! Both sources and constants can be substituted. Sources will be replaced
//! with their path and constants with their integer value. The syntax allows
//! for some simple formatting for constants.
//!
//! The syntax is mostly standard. A substitution begins with a dollar-sign
//! and is followed by the source or constant name in parentheses. For instance,
//! "$(mysource)" or "$(myconst)". The parentheses are always required.
//!
//! Constant names can be prefixed by a single formatting character followed
//! by a colon. The only formatting characters currently supported are 'd' for
//! decimal and 'x' for hex. For example, "$(x:myconst)" will be replaced with
//! the value of the constant named "myconst" formatted as hexadecimal. The
//! default is decimal, so the 'd' formatting character isn't really ever
//! needed.
//!
//! \param message The string to perform substitution on.
//! \return Returns a newly allocated std::string object that has all
//!		substitutions replaced with the associated value. The caller is
//!		responsible for freeing the string object using the delete operator.
std::string * ConversionController::substituteVariables(const std::string * message)
{
	string * result = new string();
	int i;
	int state = 0;
	string name;
	
	for (i=0; i < message->size(); ++i)
	{
		char c = (*message)[i];
		switch (state)
		{
			case 0:
				if (c == '$')
				{
					state = 1;
				}
				else
				{
					(*result) += c;
				}
				break;
			
			case 1:
				if (c == '(')
				{
					state = 2;
				}
				else
				{
					// Wasn't a variable substitution, so revert to initial state after
					// inserting the original characters.
					(*result) += '$';
					(*result) += c;
					state = 0;
				}
				break;
			
			case 2:
				if (c == ')')
				{
					// Try the name as a source name first.
					if (m_sources.find(name) != m_sources.end())
					{
						(*result) += m_sources[name]->getPath();
					}
					// Otherwise try it as a variable.
					else
					{
						// Select format.
						const char * fmt = "%d";
						if (name[1] == ':' && (name[0] == 'd' || name[0] == 'x'))
						{
							if (name[0] == 'x')
							{
								fmt = "0x%x";
							}
							
							// Delete the format characters.
							name.erase(0, 2);
						}
						
						// Now insert the formatted variable if it exists.
						if (m_context.isVariableDefined(name))
						{
							(*result) += format_string(fmt, m_context.getVariableValue(name));
						}
					}
					
					// Switch back to initial state and clear name.
					state = 0;
					name.clear();
				}
				else
				{
					// Just keep building up the variable name.
					name += c;
				}
				break;
		}
	}
	
	return result;
}

//!
//! \param generator The generator to use.
BootImage * ConversionController::generateOutput(BootImageGenerator * generator)
{
	// set the generator's option context
	generator->setOptionContext(this);
	
	// add output sections to the generator in sequence
	section_vector_t::iterator it = m_outputSections.begin();
	for (; it != m_outputSections.end(); ++it)
	{
		generator->addOutputSection(*it);
	}
	
	// and produce the output
	BootImage * image = generator->generate();
//	Log::log("boot image = %p\n", image);
	return image;
}

//! Takes an AST node that is one of the following subclasses and creates the corresponding
//! type of DataSource object from it.
//! - StringConstASTNode
//! - ExprASTNode
//! - SourceASTNode
//! - SectionASTNode
//! - SectionMatchListASTNode
//! - BlobConstASTNode
//! - IVTConstASTNode
//!
//! \exception elftosb::semantic_error Thrown if a semantic problem is found with
//!		the data node.
//! \exception std::runtime_error Thrown if an error occurs that shouldn't be possible
//!		based on the grammar.
DataSource * ConversionController::createSourceFromNode(ASTNode * dataNode)
{
	assert(dataNode);
	
	DataSource * source = NULL;
	StringConstASTNode * stringNode;
	BlobConstASTNode * blobNode;
	ExprASTNode * exprNode;
	SourceASTNode * sourceNode;
	SectionASTNode * sectionNode;
	SectionMatchListASTNode * matchListNode;
    IVTConstASTNode * ivtNode;
	
	if (stringNode = dynamic_cast<StringConstASTNode*>(dataNode))
	{
		// create a data source with the string contents
		std::string * stringData = stringNode->getString();
		const uint8_t * stringContents = reinterpret_cast<const uint8_t *>(stringData->c_str());
		source = new UnmappedDataSource(stringContents, static_cast<unsigned>(stringData->size()));
	}
	else if (blobNode = dynamic_cast<BlobConstASTNode*>(dataNode))
	{
		// create a data source with the raw binary data
		Blob * blob = blobNode->getBlob();
		source = new UnmappedDataSource(blob->getData(), blob->getLength());
	}
	else if (exprNode = dynamic_cast<ExprASTNode*>(dataNode))
	{
		// reduce the expression first
		exprNode = exprNode->reduce(m_context);
		IntConstExprASTNode * intNode = dynamic_cast<IntConstExprASTNode*>(exprNode);
		if (!intNode)
		{
			throw semantic_error("load pattern expression did not evaluate to an integer");
		}
		
		SizedIntegerValue intValue(intNode->getValue(), intNode->getSize());
		source = new PatternSource(intValue);
	}
	else if (sourceNode = dynamic_cast<SourceASTNode*>(dataNode))
	{
		// load the entire source contents
		SourceFile * sourceFile = getSourceFromName(sourceNode->getSourceName(), sourceNode->getFirstLine());
		source = sourceFile->createDataSource();
	}
	else if (sectionNode = dynamic_cast<SectionASTNode*>(dataNode))
	{
		// load some subset of the source
		SourceFile * sourceFile = getSourceFromName(sectionNode->getSourceName(), sectionNode->getFirstLine());
		if (!sourceFile->supportsNamedSections())
		{
			throw semantic_error(format_string("line %d: source does not support sections", sectionNode->getFirstLine()));
		}
		
		// create data source from the section name
		std::string * sectionName = sectionNode->getSectionName();
		GlobMatcher globber(*sectionName);
		source = sourceFile->createDataSource(globber);
		if (!source)
		{
			throw semantic_error(format_string("line %d: no sections match the pattern", sectionNode->getFirstLine()));
		}
	}
	else if (matchListNode = dynamic_cast<SectionMatchListASTNode*>(dataNode))
	{
		SourceFile * sourceFile = getSourceFromName(matchListNode->getSourceName(), matchListNode->getFirstLine());
		if (!sourceFile->supportsNamedSections())
		{
			throw semantic_error(format_string("line %d: source type does not support sections", matchListNode->getFirstLine()));
		}
		
		// create string matcher
		ExcludesListMatcher matcher;
		
		// add each pattern to the matcher
		ListASTNode * matchList = matchListNode->getSections();
		ListASTNode::iterator it = matchList->begin();
		for (; it != matchList->end(); ++it)
		{
			ASTNode * node = *it;
			sectionNode = dynamic_cast<SectionASTNode*>(node);
			if (!sectionNode)
			{
				throw std::runtime_error(format_string("line %d: unexpected node type in section pattern list", (*it)->getFirstLine()));
			}
			bool isInclude = sectionNode->getAction() == SectionASTNode::kInclude;
			matcher.addPattern(isInclude, *(sectionNode->getSectionName()));
		}
		
		// create data source from the section match list
		source = sourceFile->createDataSource(matcher);
		if (!source)
		{
			throw semantic_error(format_string("line %d: no sections match the section pattern list", matchListNode->getFirstLine()));
		}
	}
    else if (ivtNode = dynamic_cast<IVTConstASTNode*>(dataNode))
    {
        source = createIVTDataSource(ivtNode);
    }
	else
	{
		throw semantic_error(format_string("line %d: unexpected load data node type", dataNode->getFirstLine()));
	}
	
	return source;
}

DataSource * ConversionController::createIVTDataSource(IVTConstASTNode * ivtNode)
{
    IVTDataSource * source = new IVTDataSource;
    
    // Iterate over the assignment statements in the IVT definition.
    ListASTNode * fieldList = ivtNode->getFieldAssignments();
    
    if (fieldList)
    {
        ListASTNode::iterator it = fieldList->begin();
        for (; it != fieldList->end(); ++it)
        {
            AssignmentASTNode * assignmentNode = dynamic_cast<AssignmentASTNode*>(*it);
            if (!assignmentNode)
            {
                throw std::runtime_error(format_string("line %d: unexpected node type in IVT definition", (*it)->getFirstLine()));
            }
            
            // Get the IVT field name.
            std::string * fieldName = assignmentNode->getIdent();
            
            // Reduce the field expression and get the integer result.
            ASTNode * valueNode = assignmentNode->getValue();
            ExprASTNode * valueExpr = dynamic_cast<ExprASTNode*>(valueNode);
            if (!valueExpr)
            {
                throw semantic_error("IVT field must have a valid expression");
            }
            IntConstExprASTNode * valueIntExpr = dynamic_cast<IntConstExprASTNode*>(valueExpr->reduce(m_context));
            if (!valueIntExpr)
            {
                throw semantic_error(format_string("line %d: IVT field '%s' does not evaluate to an integer", valueNode->getFirstLine(), fieldName->c_str()));
            }
            uint32_t value = static_cast<uint32_t>(valueIntExpr->getValue());
            
            // Set the field in the IVT data source.
            if (!source->setFieldByName(*fieldName, value))
            {
                throw semantic_error(format_string("line %d: unknown IVT field '%s'", assignmentNode->getFirstLine(), fieldName->c_str()));
            }
        }
    }
    
    return source;
}

//! Takes an AST node subclass and returns an appropriate DataTarget object that contains
//! the same information. Supported AST node types are:
//! - SymbolASTNode
//! - NaturalLocationASTNode
//! - AddressRangeASTNode
//!
//! \exception elftosb::semantic_error Thrown if a semantic problem is found with
//!		the target node.
DataTarget * ConversionController::createTargetFromNode(ASTNode * targetNode)
{
	assert(targetNode);
	
	DataTarget * target = NULL;
	SymbolASTNode * symbolNode;
	NaturalLocationASTNode * naturalNode;
	AddressRangeASTNode * addressNode;
	
	if (symbolNode = dynamic_cast<SymbolASTNode*>(targetNode))
	{
		SourceFile * sourceFile = getSourceFromName(symbolNode->getSource(), symbolNode->getFirstLine());
		std::string * symbolName = symbolNode->getSymbolName();
		
		// symbol name is optional
		if (symbolName)
		{
			if (!sourceFile->supportsNamedSymbols())
			{
				throw std::runtime_error(format_string("line %d: source does not support symbols", symbolNode->getFirstLine()));
			}
			
			target = sourceFile->createDataTargetForSymbol(*symbolName);
			if (!target)
			{
				throw std::runtime_error(format_string("line %d: source does not have a symbol with that name", symbolNode->getFirstLine()));
			}
		}
		else
		{
			// no symbol name was specified so use entry point
			target = sourceFile->createDataTargetForEntryPoint();
			if (!target)
			{
				throw std::runtime_error(format_string("line %d: source does not have an entry point", symbolNode->getFirstLine()));
			}
		}
	}
	else if (naturalNode = dynamic_cast<NaturalLocationASTNode*>(targetNode))
	{
		// the target is the source's natural location
		target = new NaturalDataTarget();
	}
	else if (addressNode = dynamic_cast<AddressRangeASTNode*>(targetNode))
	{
		// evaluate begin address
		ExprASTNode * beginExpr = dynamic_cast<ExprASTNode*>(addressNode->getBegin());
		if (!beginExpr)
		{
			throw semantic_error("address range must always have a beginning expression");
		}
		IntConstExprASTNode * beginIntExpr = dynamic_cast<IntConstExprASTNode*>(beginExpr->reduce(m_context));
		if (!beginIntExpr)
		{
			throw semantic_error("address range begin did not evaluate to an integer");
		}
		uint32_t beginAddress = static_cast<uint32_t>(beginIntExpr->getValue());
		
		// evaluate end address
		ExprASTNode * endExpr = dynamic_cast<ExprASTNode*>(addressNode->getEnd());
		uint32_t endAddress = 0;
		bool hasEndAddress = false;
		if (endExpr)
		{
			IntConstExprASTNode * endIntExpr = dynamic_cast<IntConstExprASTNode*>(endExpr->reduce(m_context));
			if (!endIntExpr)
			{
				throw semantic_error("address range end did not evaluate to an integer");
			}
			endAddress = static_cast<uint32_t>(endIntExpr->getValue());
			hasEndAddress = true;
		}
		
		// create target
		if (hasEndAddress)
		{
			target = new ConstantDataTarget(beginAddress, endAddress);
		}
		else
		{
			target = new ConstantDataTarget(beginAddress);
		}
	}
	else
	{
		throw semantic_error("unexpected load target node type");
	}
	
	return target;
}

//! \param sourceName Pointer to string containing the name of the source to look up.
//!		May be NULL, in which case the default source is used.
//! \param line The line number on which the source name was located.
//!
//! \result A source file object that was previously created in the processSources()
//!		stage.
//!
//! \exception std::runtime_error Thrown if the source name is invalid, or if it
//!		was NULL and there is no default source (i.e., we're not inside a from
//!		statement).
SourceFile * ConversionController::getSourceFromName(std::string * sourceName, int line)
{
	SourceFile * sourceFile = NULL;
	if (sourceName)
	{
		// look up source in map
		source_map_t::iterator it = m_sources.find(*sourceName);
		if (it == m_sources.end())
		{
			source_name_vector_t::const_iterator findIt = std::find<source_name_vector_t::const_iterator, std::string>(m_failedSources.begin(), m_failedSources.end(), *sourceName);
			if (findIt != m_failedSources.end())
			{
				throw semantic_error(format_string("line %d: error opening source '%s'", line, sourceName->c_str()));
			}
			else
			{
				throw semantic_error(format_string("line %d: invalid source name '%s'", line, sourceName->c_str()));
			}
		}
		sourceFile = it->second;
	}
	else
	{
		// no name provided - use default source
		sourceFile = m_defaultSource;
		if (!sourceFile)
		{
			throw semantic_error(format_string("line %d: source required but no default source is available", line));
		}
	}
	
	// open the file if it hasn't already been
	if (!sourceFile->isOpen())
	{
		sourceFile->open();
	}
	return sourceFile;
}

//! Exercises the lexer by printing out the value of every token produced by the
//! lexer. It is assumed that the lexer object has already be configured to read
//! from some input file. The method will return when the lexer has exhausted all
//! tokens, or an error occurs.
void ConversionController::testLexer(ElftosbLexer & lexer)
{
	// test lexer
	while (1)
	{
		YYSTYPE value;
		int lexresult = lexer.yylex();
		if (lexresult == 0)
			break;
		lexer.getSymbolValue(&value);
		Log::log("%d -> int:%d, ast:%p", lexresult, value.m_int, value.m_str, value.m_ast);
		if (lexresult == TOK_IDENT || lexresult == TOK_SOURCE_NAME || lexresult == TOK_STRING_LITERAL)
		{
			if (value.m_str)
			{
				Log::log(", str:%s\n", value.m_str->c_str());
			}
			else
			{
				Log::log("str:NULL\n");
			}
		}
		else
		{
			Log::log("\n");
		}
	}
}

//! Prints out the value of an integer constant expression AST node. Also prints
//! the name of the identifier associated with that node, as well as the integer
//! size.
void ConversionController::printIntConstExpr(const std::string & ident, IntConstExprASTNode * expr)
{
	// print constant value
	char sizeChar;
	switch (expr->getSize())
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
	Log::log("%s => %d:%c\n", ident.c_str(), expr->getValue(), sizeChar);
}

