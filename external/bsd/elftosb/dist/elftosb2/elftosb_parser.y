/*
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

/* write header with token defines */
%defines

/* make it reentrant */
%pure-parser

/* put more info in error messages */
%error-verbose

/* enable location processing */
%locations

%{
#include "ElftosbLexer.h"
#include "ElftosbAST.h"
#include "Logging.h"
#include "Blob.h"
#include "format_string.h"
#include "Value.h"
#include "ConversionController.h"

using namespace elftosb;

//! Our special location type.
#define YYLTYPE token_loc_t

// this indicates that we're using our own type. it should be unset automatically
// but that's not working for some reason with the .hpp file.
#if defined(YYLTYPE_IS_TRIVIAL)
	#undef YYLTYPE_IS_TRIVIAL
	#define YYLTYPE_IS_TRIVIAL 0
#endif

//! Default location action
#define YYLLOC_DEFAULT(Current, Rhs, N)	\
	do {		\
		if (N)	\
		{		\
			(Current).m_firstLine = YYRHSLOC(Rhs, 1).m_firstLine;	\
			(Current).m_lastLine = YYRHSLOC(Rhs, N).m_lastLine;		\
		}		\
		else	\
		{		\
			(Current).m_firstLine = (Current).m_lastLine = YYRHSLOC(Rhs, 0).m_lastLine;	\
		}		\
	} while (0)

//! Forward declaration of yylex().
static int yylex(YYSTYPE * lvalp, YYLTYPE * yylloc, ElftosbLexer * lexer);

// Forward declaration of error handling function.
static void yyerror(YYLTYPE * yylloc, ElftosbLexer * lexer, CommandFileASTNode ** resultAST, const char * error);

%}

/* symbol types */
%union {
	int m_num;
	elftosb::SizedIntegerValue * m_int;
	Blob * m_blob;
	std::string * m_str;
	elftosb::ASTNode * m_ast;	// must use full name here because this is put into *.tab.hpp
}

/* extra parameters for the parser and lexer */
%parse-param	{ElftosbLexer * lexer}
%parse-param	{CommandFileASTNode ** resultAST}
%lex-param		{ElftosbLexer * lexer}

/* token definitions */
%token <m_str> TOK_IDENT			"identifier"
%token <m_str> TOK_STRING_LITERAL	"string"
%token <m_int> TOK_INT_LITERAL		"integer"
%token <m_str> TOK_SECTION_NAME		"section name"
%token <m_str> TOK_SOURCE_NAME		"source name"
%token <m_blob> TOK_BLOB			"binary object"
%token '('
%token ')'
%token '{'
%token '}'
%token '['
%token ']'
%token '='
%token ','
%token ';'
%token ':'
%token '>'
%token '.'
%token TOK_DOT_DOT				".."
%token '~'
%token '&'
%token '|'
%token '<'
%token '>'
%token '!'
%token TOK_AND					"&&"
%token TOK_OR					"||"
%token TOK_GEQ					">="
%token TOK_LEQ					"<="
%token TOK_EQ					"=="
%token TOK_NEQ					"!="
%token TOK_POWER				"**"
%token TOK_LSHIFT				"<<"
%token TOK_RSHIFT				">>"
%token <m_int> TOK_INT_SIZE		"integer size"
%token TOK_OPTIONS		"options"
%token TOK_CONSTANTS	"constants"
%token TOK_SOURCES		"sources"
%token TOK_FILTERS		"filters"
%token TOK_SECTION		"section"
%token TOK_EXTERN		"extern"
%token TOK_FROM			"from"
%token TOK_RAW			"raw"
%token TOK_LOAD			"load"
%token TOK_JUMP			"jump"
%token TOK_CALL			"call"
%token TOK_MODE			"mode"
%token TOK_IF			"if"
%token TOK_ELSE			"else"
%token TOK_DEFINED		"defined"
%token TOK_INFO			"info"
%token TOK_WARNING		"warning"
%token TOK_ERROR		"error"
%token TOK_SIZEOF		"sizeof"
%token TOK_DCD			"dcd"
%token TOK_HAB			"hab"
%token TOK_IVT			"ivt"

/* operator precedence */
%left "&&" "||"
%left '>' '<' ">=" "<=" "==" "!="
%left '|'
%left '^'
%left '&'
%left "<<" ">>"
%left "**"
%left '+' '-'
%left '*' '/' '%'
%left '.'
%right UNARY_OP

/* nonterminal types - most nonterminal symbols are subclasses of ASTNode */
%type <m_ast> command_file blocks_list pre_section_block options_block const_def_list const_def_list_elem
%type <m_ast> const_def const_expr expr int_const_expr unary_expr int_value constants_block
%type <m_ast> sources_block source_def_list source_def_list_elem source_def
%type <m_ast> section_defs section_def section_contents full_stmt_list full_stmt_list_elem
%type <m_ast> basic_stmt load_stmt call_stmt from_stmt load_data load_target call_target
%type <m_ast> address_or_range load_target_opt call_arg_opt basic_stmt_list basic_stmt_list_elem
%type <m_ast> source_attr_list source_attr_list_elem source_attrs_opt
%type <m_ast> section_list section_list_elem symbol_ref mode_stmt
%type <m_ast> section_options_opt source_attr_list_opt
%type <m_ast> if_stmt else_opt message_stmt
%type <m_ast> bool_expr ivt_def assignment_list_opt

%type <m_num> call_or_jump dcd_opt

%destructor { delete $$; } TOK_IDENT TOK_STRING_LITERAL TOK_SECTION_NAME TOK_SOURCE_NAME TOK_BLOB TOK_INT_SIZE TOK_INT_LITERAL

%%

command_file	:	blocks_list section_defs
						{
							CommandFileASTNode * commandFile = new CommandFileASTNode();
							commandFile->setBlocks(dynamic_cast<ListASTNode*>($1));
							commandFile->setSections(dynamic_cast<ListASTNode*>($2));
							commandFile->setLocation(@1, @2);
							*resultAST = commandFile;
						}
				;

blocks_list		:	pre_section_block
						{
							ListASTNode * list = new ListASTNode();
							list->appendNode($1);
							$$ = list;
						}
				|	blocks_list pre_section_block
						{
							dynamic_cast<ListASTNode*>($1)->appendNode($2);
							$$ = $1;
						}
				;

pre_section_block
				:	options_block			{ $$ = $1; }
				|	constants_block			{ $$ = $1; }
				|	sources_block			{ $$ = $1; }
				;

options_block		:	"options" '{' const_def_list '}'
							{
								$$ = new OptionsBlockASTNode(dynamic_cast<ListASTNode *>($3));
							}
					;

constants_block		:	"constants" '{' const_def_list '}'
							{
								$$ = new ConstantsBlockASTNode(dynamic_cast<ListASTNode *>($3));
							}
					;

const_def_list		:	const_def_list_elem
							{
								ListASTNode * list = new ListASTNode();
								list->appendNode($1);
								$$ = list;
							}
					|	const_def_list const_def_list_elem
							{
								dynamic_cast<ListASTNode*>($1)->appendNode($2);
								$$ = $1;
							}
					;

const_def_list_elem	:	const_def ';'		{ $$ = $1; }
					|	/* empty */			{ $$ = NULL; }
					;

const_def			:	TOK_IDENT '=' const_expr
							{
								$$ = new AssignmentASTNode($1, $3);
								$$->setLocation(@1, @3);
							}
					;

sources_block	:	"sources" '{' source_def_list '}'
						{
							$$ = new SourcesBlockASTNode(dynamic_cast<ListASTNode *>($3));
						}
				;

source_def_list	:	source_def_list_elem
						{
							ListASTNode * list = new ListASTNode();
							list->appendNode($1);
							$$ = list;
						}
				|	source_def_list source_def_list_elem
						{
							dynamic_cast<ListASTNode*>($1)->appendNode($2);
							$$ = $1;
						}
				;

source_def_list_elem
				:		source_def source_attrs_opt ';'
							{
								// tell the lexer that this is the name of a source file
								SourceDefASTNode * node = dynamic_cast<SourceDefASTNode*>($1);
								if ($2)
								{
									node->setAttributes(dynamic_cast<ListASTNode*>($2));
								}
								node->setLocation(node->getLocation(), @3);
								lexer->addSourceName(node->getName());
								$$ = $1;
							}
				|		/* empty */		{ $$ = NULL; }
				;

source_def		:		TOK_IDENT '=' TOK_STRING_LITERAL
							{
								$$ = new PathSourceDefASTNode($1, $3);
								$$->setLocation(@1, @3);
							}
				|		TOK_IDENT '=' "extern" '(' int_const_expr ')'
							{
								$$ = new ExternSourceDefASTNode($1, dynamic_cast<ExprASTNode*>($5));
								$$->setLocation(@1, @6);
							}
				;

source_attrs_opt
				:		'(' source_attr_list ')'		{ $$ = $2; }
				|		/* empty */						{ $$ = NULL; }
				;

source_attr_list
				:		source_attr_list_elem
							{
								ListASTNode * list = new ListASTNode();
								list->appendNode($1);
								$$ = list;
							}
				|		source_attr_list ',' source_attr_list_elem
							{
								dynamic_cast<ListASTNode*>($1)->appendNode($3);
								$$ = $1;
							}
				;
						
source_attr_list_elem
				:		TOK_IDENT '=' const_expr
							{
								$$ = new AssignmentASTNode($1, $3);
								$$->setLocation(@1, @3);
							}
				;

section_defs	:		section_def
							{
								ListASTNode * list = new ListASTNode();
								list->appendNode($1);
								$$ = list;
							}
				|		section_defs section_def
							{
								dynamic_cast<ListASTNode*>($1)->appendNode($2);
								$$ = $1;
							}
				;

section_def		:		"section" '(' int_const_expr section_options_opt ')' section_contents
							{
								SectionContentsASTNode * sectionNode = dynamic_cast<SectionContentsASTNode*>($6);
								if (sectionNode)
								{
									ExprASTNode * exprNode = dynamic_cast<ExprASTNode*>($3);
									sectionNode->setSectionNumberExpr(exprNode);
									sectionNode->setOptions(dynamic_cast<ListASTNode*>($4));
									sectionNode->setLocation(@1, sectionNode->getLocation());
								}
								$$ = $6;
							}
				;

section_options_opt
				:		';' source_attr_list_opt
							{
								$$ = $2;
							}
				|		/* empty */
							{
								$$ = NULL;
							}
				;

source_attr_list_opt
				:		source_attr_list
							{
								$$ = $1;
							}
				|		/* empty */
							{
								$$ = NULL;
							}
				;

section_contents
				:		"<=" load_data ';'
							{
								DataSectionContentsASTNode * dataSection = new DataSectionContentsASTNode($2);
								dataSection->setLocation(@1, @3);
								$$ = dataSection;
							}
				|		'{' full_stmt_list '}'
							{
								ListASTNode * listNode = dynamic_cast<ListASTNode*>($2);
								$$ = new BootableSectionContentsASTNode(listNode);
								$$->setLocation(@1, @3);
							}
				;

full_stmt_list	:		full_stmt_list_elem
							{
								ListASTNode * list = new ListASTNode();
								list->appendNode($1);
								$$ = list;
							}
				|		full_stmt_list full_stmt_list_elem
							{
								dynamic_cast<ListASTNode*>($1)->appendNode($2);
								$$ = $1;
							}
				;

full_stmt_list_elem
				:		basic_stmt ';'		{ $$ = $1; }
				|		from_stmt			{ $$ = $1; }
				|		if_stmt				{ $$ = $1; }
				|		/* empty */			{ $$ = NULL; }
				;

basic_stmt_list	:		basic_stmt_list_elem
							{
								ListASTNode * list = new ListASTNode();
								list->appendNode($1);
								$$ = list;
							}
				|		basic_stmt_list basic_stmt_list_elem
							{
								dynamic_cast<ListASTNode*>($1)->appendNode($2);
								$$ = $1;
							}
				;

basic_stmt_list_elem
				:		basic_stmt ';'		{ $$ = $1; }
				|		if_stmt				{ $$ = $1; }
				|		/* empty */			{ $$ = NULL; }
				;

basic_stmt		:		load_stmt		{ $$ = $1; }
				|		call_stmt		{ $$ = $1; }
				|		mode_stmt		{ $$ = $1; }
				|		message_stmt	{ $$ = $1; }
				;

load_stmt		:		"load" dcd_opt load_data load_target_opt
							{
								LoadStatementASTNode * stmt = new LoadStatementASTNode();
								stmt->setData($3);
								stmt->setTarget($4);
								// set dcd load flag if the "dcd" keyword was present.
								if ($2)
								{
									stmt->setDCDLoad(true);
								}
								// set char locations for the statement
								if ($4)
								{
									stmt->setLocation(@1, @4);
								}
								else
								{
									stmt->setLocation(@1, @3);
								}
								$$ = stmt;
							}
				;

dcd_opt			:		"dcd"
							{
								if (!elftosb::g_enableHABSupport)
								{
									yyerror(&yylloc, lexer, resultAST, "HAB features not supported with the selected family");
									YYABORT;
								}
								
								$$ = 1;
							}
				|		/* empty */			{ $$ = 0; }

load_data		:		int_const_expr
							{
								$$ = $1;
							}
				|		TOK_STRING_LITERAL
							{
								$$ = new StringConstASTNode($1);
								$$->setLocation(@1);
							}
				|		TOK_SOURCE_NAME
							{
								$$ = new SourceASTNode($1);
								$$->setLocation(@1);
							}
				|		section_list
							{
								$$ = new SectionMatchListASTNode(dynamic_cast<ListASTNode*>($1));
								$$->setLocation(@1);
							}
				|		section_list "from" TOK_SOURCE_NAME
							{
								$$ = new SectionMatchListASTNode(dynamic_cast<ListASTNode*>($1), $3);
								$$->setLocation(@1, @3);
							}
				|		TOK_SOURCE_NAME '[' section_list ']'
							{
								$$ = new SectionMatchListASTNode(dynamic_cast<ListASTNode*>($3), $1);
								$$->setLocation(@1, @4);
							}
				|		TOK_BLOB
							{
								$$ = new BlobConstASTNode($1);
								$$->setLocation(@1);
							}
				|		ivt_def
							{
							}
				;

section_list	:		section_list_elem
							{
								ListASTNode * list = new ListASTNode();
								list->appendNode($1);
								$$ = list;
							}
				|		section_list ',' section_list_elem
							{
								dynamic_cast<ListASTNode*>($1)->appendNode($3);
								$$ = $1;
							}
				;

section_list_elem
				:		TOK_SECTION_NAME
							{
								$$ = new SectionASTNode($1, SectionASTNode::kInclude);
								$$->setLocation(@1);
							}
				|		'~' TOK_SECTION_NAME
							{
								$$ = new SectionASTNode($2, SectionASTNode::kExclude);
								$$->setLocation(@1, @2);
							}
				;

load_target_opt	:		'>' load_target
							{
								$$ = $2;
							}
				|		/* empty */
							{
								$$ = new NaturalLocationASTNode();
//								$$->setLocation();
							}
				;

load_target		:		'.'
							{
								$$ = new NaturalLocationASTNode();
								$$->setLocation(@1);
							}
				|		address_or_range
							{
								$$ = $1;
							}
				;

ivt_def			:		"ivt" '(' assignment_list_opt ')'
							{
								IVTConstASTNode * ivt = new IVTConstASTNode();
								if ($3)
								{
									ivt->setFieldAssignments(dynamic_cast<ListASTNode*>($3));
								}
								ivt->setLocation(@1, @4);
								$$ = ivt;
							}
				;

assignment_list_opt	:	source_attr_list		{ $$ = $1; }
					|	/* empty */				{ $$ = NULL; }
					;

call_stmt		:		call_or_jump call_target call_arg_opt
							{
								CallStatementASTNode * stmt = new CallStatementASTNode();
								switch ($1)
								{
									case 1:
										stmt->setCallType(CallStatementASTNode::kCallType);
										break;
									case 2:
										stmt->setCallType(CallStatementASTNode::kJumpType);
										break;
									default:
										yyerror(&yylloc, lexer, resultAST, "invalid call_or_jump value");
										YYABORT;
										break;
								}
								stmt->setTarget($2);
								stmt->setArgument($3);
								stmt->setIsHAB(false);
								if ($3)
								{
									stmt->setLocation(@1, @3);
								}
								else
								{
									stmt->setLocation(@1, @2);
								}
								$$ = stmt;
							}
				|		"hab" call_or_jump address_or_range call_arg_opt
							{
								if (!elftosb::g_enableHABSupport)
								{
									yyerror(&yylloc, lexer, resultAST, "HAB features not supported with the selected family");
									YYABORT;
								}
								
								CallStatementASTNode * stmt = new CallStatementASTNode();
								switch ($2)
								{
									case 1:
										stmt->setCallType(CallStatementASTNode::kCallType);
										break;
									case 2:
										stmt->setCallType(CallStatementASTNode::kJumpType);
										break;
									default:
										yyerror(&yylloc, lexer, resultAST, "invalid call_or_jump value");
										YYABORT;
										break;
								}
								stmt->setTarget($3);
								stmt->setArgument($4);
								stmt->setIsHAB(true);
								if ($4)
								{
									stmt->setLocation(@1, @4);
								}
								else
								{
									stmt->setLocation(@1, @3);
								}
								$$ = stmt;
							}
				;

call_or_jump	:		"call"		{ $$ = 1; }
				|		"jump"		{ $$ = 2; }
				;

call_target		:		TOK_SOURCE_NAME
							{
								$$ = new SymbolASTNode(NULL, $1);
								$$->setLocation(@1);
							}
				|		int_const_expr
							{
								$$ = new AddressRangeASTNode($1, NULL);
								$$->setLocation($1);
							}
				;

call_arg_opt	:		'(' int_const_expr ')'		{ $$ = $2; }
				|		'(' ')'						{ $$ = NULL; }
				|		/* empty */					{ $$ = NULL; }
				;

from_stmt		:		"from" TOK_SOURCE_NAME '{' basic_stmt_list '}'
							{
								$$ = new FromStatementASTNode($2, dynamic_cast<ListASTNode*>($4));
								$$->setLocation(@1, @5);
							}
				;

mode_stmt		:		"mode" int_const_expr
							{
								$$ = new ModeStatementASTNode(dynamic_cast<ExprASTNode*>($2));
								$$->setLocation(@1, @2);
							}
				;

message_stmt	:		"info" TOK_STRING_LITERAL
							{
								$$ = new MessageStatementASTNode(MessageStatementASTNode::kInfo, $2);
								$$->setLocation(@1, @2);
							}
				|		"warning" TOK_STRING_LITERAL
							{
								$$ = new MessageStatementASTNode(MessageStatementASTNode::kWarning, $2);
								$$->setLocation(@1, @2);
							}
				|		"error" TOK_STRING_LITERAL
							{
								$$ = new MessageStatementASTNode(MessageStatementASTNode::kError, $2);
								$$->setLocation(@1, @2);
							}
				;

if_stmt			:		"if" bool_expr '{' full_stmt_list '}' else_opt
							{
								IfStatementASTNode * ifStmt = new IfStatementASTNode();
								ifStmt->setConditionExpr(dynamic_cast<ExprASTNode*>($2));
								ifStmt->setIfStatements(dynamic_cast<ListASTNode*>($4));
								ifStmt->setElseStatements(dynamic_cast<ListASTNode*>($6));
								ifStmt->setLocation(@1, @6);
								$$ = ifStmt;
							}
				;

else_opt		:		"else" '{' full_stmt_list '}'
							{
								$$ = $3;
							}
				|		"else" if_stmt
							{
								ListASTNode * list = new ListASTNode();
								list->appendNode($2);
								$$ = list;
								$$->setLocation(@1, @2);
							}
				|		/* empty */			{ $$ = NULL; }
				;

address_or_range	:	int_const_expr
							{
								$$ = new AddressRangeASTNode($1, NULL);
								$$->setLocation($1);
							}
					|	int_const_expr ".." int_const_expr
							{
								$$ = new AddressRangeASTNode($1, $3);
								$$->setLocation(@1, @3);
							}
					;

const_expr		:	bool_expr
							{
								$$ = $1;
							}
				|	TOK_STRING_LITERAL
							{
								$$ = new StringConstASTNode($1);
								$$->setLocation(@1);
							}
				;

bool_expr		:	int_const_expr
						{
							$$ = $1;
						}
				|	bool_expr '<' bool_expr
						{
							ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
							ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
							$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kLessThan, right);
							$$->setLocation(@1, @3);
						}
				|	bool_expr '>' bool_expr
						{
							ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
							ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
							$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kGreaterThan, right);
							$$->setLocation(@1, @3);
						}
				|	bool_expr ">=" bool_expr
						{
							ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
							ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
							$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kGreaterThanEqual, right);
							$$->setLocation(@1, @3);
						}
				|	bool_expr "<=" bool_expr
						{
							ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
							ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
							$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kLessThanEqual, right);
							$$->setLocation(@1, @3);
						}
				|	bool_expr "==" bool_expr
						{
							ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
							ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
							$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kEqual, right);
							$$->setLocation(@1, @3);
						}
				|	bool_expr "!=" bool_expr
						{
							ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
							ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
							$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kNotEqual, right);
							$$->setLocation(@1, @3);
						}
				|	bool_expr "&&" bool_expr
						{
							ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
							ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
							$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBooleanAnd, right);
							$$->setLocation(@1, @3);
						}
				|	bool_expr "||" bool_expr
						{
							ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
							ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
							$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBooleanOr, right);
							$$->setLocation(@1, @3);
						}
				|	'!' bool_expr %prec UNARY_OP
						{
							$$ = new BooleanNotExprASTNode(dynamic_cast<ExprASTNode*>($2));
							$$->setLocation(@1, @2);
						}
				|	TOK_IDENT '(' TOK_SOURCE_NAME ')'
						{
							$$ = new SourceFileFunctionASTNode($1, $3);
							$$->setLocation(@1, @4);
						}
				|	'(' bool_expr ')'
						{
							$$ = $2;
							$$->setLocation(@1, @3);
						}
				|	"defined" '(' TOK_IDENT ')'
						{
							$$ = new DefinedOperatorASTNode($3);
							$$->setLocation(@1, @4);
						}
				;

int_const_expr	:	expr				{ $$ = $1; }
				;

symbol_ref		:	TOK_SOURCE_NAME ':' TOK_IDENT
							{
								$$ = new SymbolASTNode($3, $1);
								$$->setLocation(@1, @3);
							}
				|	':' TOK_IDENT
							{
								$$ = new SymbolASTNode($2);
								$$->setLocation(@1, @2);
							}
				;


expr			:		int_value
							{
								$$ = $1;
							}
				|		TOK_IDENT
							{
								$$ = new VariableExprASTNode($1);
								$$->setLocation(@1);
							}
				|		symbol_ref
							{
								$$ = new SymbolRefExprASTNode(dynamic_cast<SymbolASTNode*>($1));
								$$->setLocation(@1);
							}
/*				|		expr '..' expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new RangeExprASTNode(left, right);
							}
*/				|		expr '+' expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kAdd, right);
								$$->setLocation(@1, @3);
							}
				|		expr '-' expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kSubtract, right);
								$$->setLocation(@1, @3);
							}
				|		expr '*' expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kMultiply, right);
								$$->setLocation(@1, @3);
							}
				|		expr '/' expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kDivide, right);
								$$->setLocation(@1, @3);
							}
				|		expr '%' expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kModulus, right);
								$$->setLocation(@1, @3);
							}
				|		expr "**" expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kPower, right);
								$$->setLocation(@1, @3);
							}
				|		expr '&' expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBitwiseAnd, right);
								$$->setLocation(@1, @3);
							}
				|		expr '|' expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBitwiseOr, right);
								$$->setLocation(@1, @3);
							}
				|		expr '^' expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kBitwiseXor, right);
								$$->setLocation(@1, @3);
							}
				|		expr "<<" expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kShiftLeft, right);
								$$->setLocation(@1, @3);
							}
				|		expr ">>" expr
							{
								ExprASTNode * left = dynamic_cast<ExprASTNode*>($1);
								ExprASTNode * right = dynamic_cast<ExprASTNode*>($3);
								$$ = new BinaryOpExprASTNode(left, BinaryOpExprASTNode::kShiftRight, right);
								$$->setLocation(@1, @3);
							}
				|		unary_expr
							{
								$$ = $1;
							}
				|		expr '.' TOK_INT_SIZE	
							{
								$$ = new IntSizeExprASTNode(dynamic_cast<ExprASTNode*>($1), $3->getWordSize());
								$$->setLocation(@1, @3);
							}
				|		'(' expr ')'
							{
								$$ = $2;
								$$->setLocation(@1, @3);
							}
				|	"sizeof" '(' symbol_ref ')'
						{
							$$ = new SizeofOperatorASTNode(dynamic_cast<SymbolASTNode*>($3));
							$$->setLocation(@1, @4);
						}
				|	"sizeof" '(' TOK_IDENT ')'
						{
							$$ = new SizeofOperatorASTNode($3);
							$$->setLocation(@1, @4);
						}
				|	"sizeof" '(' TOK_SOURCE_NAME ')'
						{
							$$ = new SizeofOperatorASTNode($3);
							$$->setLocation(@1, @4);
						}
				;

unary_expr		:		'+' expr %prec UNARY_OP
							{
								$$ = $2;
							}
				|		'-' expr %prec UNARY_OP
							{
								$$ = new NegativeExprASTNode(dynamic_cast<ExprASTNode*>($2));
								$$->setLocation(@1, @2);
							}
				;

int_value		:		TOK_INT_LITERAL
							{
								$$ = new IntConstExprASTNode($1->getValue(), $1->getWordSize());
								$$->setLocation(@1);
							}
				;

%%

/* code goes here */

static int yylex(YYSTYPE * lvalp, YYLTYPE * yylloc, ElftosbLexer * lexer)
{
	int token = lexer->yylex();
	*yylloc = lexer->getLocation();
	lexer->getSymbolValue(lvalp);
	return token;
}

static void yyerror(YYLTYPE * yylloc, ElftosbLexer * lexer, CommandFileASTNode ** resultAST, const char * error)
{
	throw syntax_error(format_string("line %d: %s\n", yylloc->m_firstLine, error));
}

