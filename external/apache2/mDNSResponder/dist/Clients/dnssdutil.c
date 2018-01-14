/*
	Copyright (c) 2016-2017 Apple Inc. All rights reserved.
	
	dnssdutil is a command-line utility for testing the DNS-SD API.
*/

#include <CoreUtils/CommonServices.h>	// Include early.
#include <CoreUtils/AsyncConnection.h>
#include <CoreUtils/CommandLineUtils.h>
#include <CoreUtils/DataBufferUtils.h>
#include <CoreUtils/DebugServices.h>
#include <CoreUtils/MiscUtils.h>
#include <CoreUtils/NetUtils.h>
#include <CoreUtils/PrintFUtils.h>
#include <CoreUtils/RandomNumberUtils.h>
#include <CoreUtils/StringUtils.h>
#include <CoreUtils/TickUtils.h>
#include <dns_sd.h>
#include <dns_sd_private.h>

#if( TARGET_OS_DARWIN )
	#include <libproc.h>
	#include <sys/proc_info.h>
#endif

#if( TARGET_OS_POSIX )
	#include <sys/resource.h>
#endif

#if( DNSSDUTIL_INCLUDE_DNSCRYPT )
	#include "tweetnacl.h"	// TweetNaCl from <https://tweetnacl.cr.yp.to/software.html>.
#endif

//===========================================================================================================================
//	Global Constants
//===========================================================================================================================

// Versioning

#define kDNSSDUtilNumVersion	NumVersionBuild( 2, 0, 0, kVersionStageBeta, 0 )

#if( !MDNSRESPONDER_PROJECT && !defined( DNSSDUTIL_SOURCE_VERSION ) )
	#define DNSSDUTIL_SOURCE_VERSION	"0.0.0"
#endif

// DNS-SD API flag descriptors

#define kDNSServiceFlagsDescriptors		\
	"\x00" "AutoTrigger\0"				\
	"\x01" "Add\0"						\
	"\x02" "Default\0"					\
	"\x03" "NoAutoRename\0"				\
	"\x04" "Shared\0"					\
	"\x05" "Unique\0"					\
	"\x06" "BrowseDomains\0"			\
	"\x07" "RegistrationDomains\0"		\
	"\x08" "LongLivedQuery\0"			\
	"\x09" "AllowRemoteQuery\0"			\
	"\x0A" "ForceMulticast\0"			\
	"\x0B" "KnownUnique\0"				\
	"\x0C" "ReturnIntermediates\0"		\
	"\x0D" "NonBrowsable\0"				\
	"\x0E" "ShareConnection\0"			\
	"\x0F" "SuppressUnusable\0"			\
	"\x10" "Timeout\0"					\
	"\x11" "IncludeP2P\0"				\
	"\x12" "WakeOnResolve\0"			\
	"\x13" "BackgroundTrafficClass\0"	\
	"\x14" "IncludeAWDL\0"				\
	"\x15" "Validate\0"					\
	"\x16" "UnicastResponse\0"			\
	"\x17" "ValidateOptional\0"			\
	"\x18" "WakeOnlyService\0"			\
	"\x19" "ThresholdOne\0"				\
	"\x1A" "ThresholdFinder\0"			\
	"\x1B" "DenyCellular\0"				\
	"\x1C" "ServiceIndex\0"				\
	"\x1D" "DenyExpensive\0"			\
	"\x1E" "PathEvaluationDone\0"		\
	"\x00"

#define kDNSServiceProtocolDescriptors	\
	"\x00" "IPv4\0"						\
	"\x01" "IPv6\0"						\
	"\x00"

// (m)DNS

#define kDNSHeaderFlag_Response					( 1 << 15 )
#define kDNSHeaderFlag_AuthAnswer				( 1 << 10 )
#define kDNSHeaderFlag_Truncation				( 1 <<  9 )
#define kDNSHeaderFlag_RecursionDesired			( 1 <<  8 )
#define kDNSHeaderFlag_RecursionAvailable		( 1 <<  7 )

#define kDNSOpCode_Query			0
#define kDNSOpCode_InverseQuery		1
#define kDNSOpCode_Status			2
#define kDNSOpCode_Notify			4
#define kDNSOpCode_Update			5

#define kDNSRCode_NoError				0
#define kDNSRCode_FormatError			1
#define kDNSRCode_ServerFailure			2
#define kDNSRCode_NXDomain				3
#define kDNSRCode_NotImplemented		4
#define kDNSRCode_Refused				5

#define kQClassUnicastResponseBit	( 1U << 15 )
#define kRRClassCacheFlushBit		( 1U << 15 )

#define kDomainLabelLengthMax		63
#define kDomainNameLengthMax		256

//===========================================================================================================================
//	Gerneral Command Options
//===========================================================================================================================

// Command option macros

#define Command( NAME, CALLBACK, SUB_OPTIONS, SHORT_HELP, IS_NOTCOMMON )											\
	CLI_COMMAND_EX( NAME, CALLBACK, SUB_OPTIONS, (IS_NOTCOMMON) ? kCLIOptionFlags_NotCommon : kCLIOptionFlags_None,	\
		(SHORT_HELP), NULL )

#define kRequiredOptionSuffix		" [REQUIRED]"

#define MultiStringOptionEx( SHORT_CHAR, LONG_NAME, VAL_PTR, VAL_COUNT_PTR, ARG_HELP, SHORT_HELP, IS_REQUIRED, LONG_HELP )	\
	CLI_OPTION_MULTI_STRING_EX( SHORT_CHAR, LONG_NAME, VAL_PTR, VAL_COUNT_PTR, ARG_HELP,									\
		(IS_REQUIRED) ? SHORT_HELP kRequiredOptionSuffix : SHORT_HELP,														\
		(IS_REQUIRED) ? kCLIOptionFlags_Required : kCLIOptionFlags_None, LONG_HELP )

#define MultiStringOption( SHORT_CHAR, LONG_NAME, VAL_PTR, VAL_COUNT_PTR, ARG_HELP, SHORT_HELP, IS_REQUIRED ) \
		MultiStringOptionEx( SHORT_CHAR, LONG_NAME, VAL_PTR, VAL_COUNT_PTR, ARG_HELP, SHORT_HELP, IS_REQUIRED, NULL )

#define IntegerOption( SHORT_CHAR, LONG_NAME, VAL_PTR, ARG_HELP, SHORT_HELP, IS_REQUIRED )	\
	CLI_OPTION_INTEGER_EX( SHORT_CHAR, LONG_NAME, VAL_PTR, ARG_HELP,						\
		(IS_REQUIRED) ? SHORT_HELP kRequiredOptionSuffix : SHORT_HELP,						\
		(IS_REQUIRED) ? kCLIOptionFlags_Required : kCLIOptionFlags_None, NULL )

#define BooleanOption( SHORT_CHAR, LONG_NAME, VAL_PTR, SHORT_HELP ) \
	CLI_OPTION_BOOLEAN( (SHORT_CHAR), (LONG_NAME), (VAL_PTR), (SHORT_HELP), NULL )

#define StringOptionEx( SHORT_CHAR, LONG_NAME, VAL_PTR, ARG_HELP, SHORT_HELP, IS_REQUIRED, LONG_HELP )	\
	CLI_OPTION_STRING_EX( SHORT_CHAR, LONG_NAME, VAL_PTR, ARG_HELP,										\
		(IS_REQUIRED) ? SHORT_HELP kRequiredOptionSuffix : SHORT_HELP,									\
		(IS_REQUIRED) ? kCLIOptionFlags_Required : kCLIOptionFlags_None, LONG_HELP )

#define StringOption( SHORT_CHAR, LONG_NAME, VAL_PTR, ARG_HELP, SHORT_HELP, IS_REQUIRED ) \
	StringOptionEx( SHORT_CHAR, LONG_NAME, VAL_PTR, ARG_HELP, SHORT_HELP, IS_REQUIRED, NULL )

// DNS-SD API flag options

static int		gDNSSDFlags						= 0;
static int		gDNSSDFlag_BrowseDomains		= false;
static int		gDNSSDFlag_DenyCellular			= false;
static int		gDNSSDFlag_DenyExpensive		= false;
static int		gDNSSDFlag_ForceMulticast		= false;
static int		gDNSSDFlag_IncludeAWDL			= false;
static int		gDNSSDFlag_NoAutoRename			= false;
static int		gDNSSDFlag_PathEvaluationDone	= false;
static int		gDNSSDFlag_RegistrationDomains	= false;
static int		gDNSSDFlag_ReturnIntermediates	= false;
static int		gDNSSDFlag_Shared				= false;
static int		gDNSSDFlag_SuppressUnusable		= false;
static int		gDNSSDFlag_Timeout				= false;
static int		gDNSSDFlag_UnicastResponse		= false;
static int		gDNSSDFlag_Unique				= false;

#define DNSSDFlagsOption()								\
	IntegerOption( 'f', "flags", &gDNSSDFlags, "flags",	\
		"DNSServiceFlags to use. This value is bitwise ORed with other single flag options.", false )

#define DNSSDFlagOption( SHORT_CHAR, FLAG_NAME )									\
	BooleanOption( SHORT_CHAR, Stringify( FLAG_NAME ), &gDNSSDFlag_ ## FLAG_NAME,	\
		"Use kDNSServiceFlags" Stringify( FLAG_NAME ) "." )

#define DNSSDFlagsOption_DenyCellular()			DNSSDFlagOption( 'C', DenyCellular )
#define DNSSDFlagsOption_DenyExpensive()		DNSSDFlagOption( 'E', DenyExpensive )
#define DNSSDFlagsOption_ForceMulticast()		DNSSDFlagOption( 'M', ForceMulticast )
#define DNSSDFlagsOption_IncludeAWDL()			DNSSDFlagOption( 'A', IncludeAWDL )
#define DNSSDFlagsOption_NoAutoRename()			DNSSDFlagOption( 'N', NoAutoRename )
#define DNSSDFlagsOption_PathEvalDone()			DNSSDFlagOption( 'P', PathEvaluationDone )
#define DNSSDFlagsOption_ReturnIntermediates()	DNSSDFlagOption( 'I', ReturnIntermediates )
#define DNSSDFlagsOption_Shared()				DNSSDFlagOption( 'S', Shared )
#define DNSSDFlagsOption_SuppressUnusable()		DNSSDFlagOption( 'S', SuppressUnusable )
#define DNSSDFlagsOption_Timeout()				DNSSDFlagOption( 'T', Timeout )
#define DNSSDFlagsOption_UnicastResponse()		DNSSDFlagOption( 'U', UnicastResponse )
#define DNSSDFlagsOption_Unique()				DNSSDFlagOption( 'U', Unique )

// Interface option

static const char *		gInterface = NULL;

#define InterfaceOption()										\
	StringOption( 'i', "interface", &gInterface, "interface",	\
		"Network interface by name or index. Use index -1 for local-only.", false )

// Connection options

#define kConnectionArg_Normal			""
#define kConnectionArgPrefix_PID		"pid:"
#define kConnectionArgPrefix_UUID		"uuid:"

static const char *		gConnectionOpt = kConnectionArg_Normal;

#define ConnectionOptions()																						\
	{ kCLIOptionType_String, 0, "connection", &gConnectionOpt, NULL, (intptr_t) kConnectionArg_Normal, "type",	\
		kCLIOptionFlags_OptionalArgument, NULL, NULL, NULL, NULL,												\
		"Specifies the type of main connection to use. See " kConnectionSection_Name " below.", NULL }

#define kConnectionSection_Name		"Connection Option"
#define kConnectionSection_Text																							\
	"The default behavior is to create a main connection with DNSServiceCreateConnection() and perform operations on\n"	\
	"the main connection using the kDNSServiceFlagsShareConnection flag. This behavior can be explicitly invoked by\n"	\
	"specifying the connection option without an argument, i.e.,\n"														\
	"\n"																												\
	"    --connection\n"																								\
	"\n"																												\
	"To instead use a delegate connection created with DNSServiceCreateDelegateConnection(), use\n"						\
	"\n"																												\
	"    --connection=pid:<PID>\n"																						\
	"\n"																												\
	"to specify the delegator by PID, or use\n"																			\
	"\n"																												\
	"    --connection=uuid:<UUID>\n"																					\
	"\n"																												\
	"to specify the delegator by UUID.\n"																				\
	"\n"																												\
	"To not use a main connection at all, but instead perform operations on their own connections, use\n"				\
	"\n"																												\
	"    --no-connection\n"

#define ConnectionSection()		CLI_SECTION( kConnectionSection_Name, kConnectionSection_Text )

// Help text for record data options

#define kRDataArgPrefix_File			"file:"
#define kRDataArgPrefix_HexString		"hex:"
#define kRDataArgPrefix_String			"string:"
#define kRDataArgPrefix_TXT				"txt:"

#define kRecordDataSection_Name		"Record Data Arguments"
#define kRecordDataSection_Text																							\
	"A record data argument is specified in one of the following formats:\n"											\
	"\n"																												\
	"Format                           Syntax                                 Example\n"									\
	"String                           string:<string>                        string:'\\x09color=red'\n"					\
	"Hexadecimal string               hex:<hex string>                       hex:c0a80101 or hex:'C0 A8 01 01'\n"		\
	"TXT record keys and values       txt:<comma-delimited keys and values>  txt:'key1=x,key2=y\\,z,key3'\n"			\
	"File containing raw record data  file:<file path>                       file:dir/record_data.bin\n"

#define RecordDataSection()		CLI_SECTION( kRecordDataSection_Name, kRecordDataSection_Text )

//===========================================================================================================================
//	Browse Command Options
//===========================================================================================================================

static char **			gBrowse_ServiceTypes		= NULL;
static size_t			gBrowse_ServiceTypesCount	= 0;
static const char *		gBrowse_Domain				= NULL;
static int				gBrowse_DoResolve			= false;
static int				gBrowse_QueryTXT			= false;
static int				gBrowse_TimeLimitSecs		= 0;

static CLIOption		kBrowseOpts[] =
{
	InterfaceOption(),
	MultiStringOption(	't', "type",	&gBrowse_ServiceTypes, &gBrowse_ServiceTypesCount, "service type", "Service type(s), e.g., \"_ssh._tcp\".", true ),
	StringOption(		'd', "domain",	&gBrowse_Domain, "domain", "Domain in which to browse for the service type(s).", false ),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption(),
	DNSSDFlagsOption_IncludeAWDL(),
	
	CLI_OPTION_GROUP( "Operation" ),
	ConnectionOptions(),
	BooleanOption(  0 , "resolve",		&gBrowse_DoResolve,		"Resolve service instances." ),
	BooleanOption(  0 , "queryTXT",		&gBrowse_QueryTXT,		"Query TXT records of service instances." ),
	IntegerOption( 'l', "timeLimit",	&gBrowse_TimeLimitSecs,	"seconds", "Specifies the max duration of the browse operation. Use '0' for no time limit.", false ),
	
	ConnectionSection(),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	GetAddrInfo Command Options
//===========================================================================================================================

static const char *		gGetAddrInfo_Name			= NULL;
static int				gGetAddrInfo_ProtocolIPv4	= false;
static int				gGetAddrInfo_ProtocolIPv6	= false;
static int				gGetAddrInfo_OneShot		= false;
static int				gGetAddrInfo_TimeLimitSecs	= 0;

static CLIOption		kGetAddrInfoOpts[] =
{
	InterfaceOption(),
	StringOption(  'n', "name", &gGetAddrInfo_Name,			"domain name", "Domain name to resolve.", true ),
	BooleanOption(  0 , "ipv4", &gGetAddrInfo_ProtocolIPv4,	"Use kDNSServiceProtocol_IPv4." ),
	BooleanOption(  0 , "ipv6", &gGetAddrInfo_ProtocolIPv6,	"Use kDNSServiceProtocol_IPv6." ),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption(),
	DNSSDFlagsOption_DenyCellular(),
	DNSSDFlagsOption_DenyExpensive(),
	DNSSDFlagsOption_PathEvalDone(),
	DNSSDFlagsOption_ReturnIntermediates(),
	DNSSDFlagsOption_SuppressUnusable(),
	DNSSDFlagsOption_Timeout(),
	
	CLI_OPTION_GROUP( "Operation" ),
	ConnectionOptions(),
	BooleanOption( 'o', "oneshot",		&gGetAddrInfo_OneShot,			"Finish after first set of results." ),
	IntegerOption( 'l', "timeLimit",	&gGetAddrInfo_TimeLimitSecs,	"seconds", "Maximum duration of the GetAddrInfo operation. Use '0' for no time limit.", false ),
	
	ConnectionSection(),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	QueryRecord Command Options
//===========================================================================================================================

static const char *		gQueryRecord_Name			= NULL;
static const char *		gQueryRecord_Type			= NULL;
static int				gQueryRecord_OneShot		= false;
static int				gQueryRecord_TimeLimitSecs	= 0;
static int				gQueryRecord_RawRData		= false;

static CLIOption		kQueryRecordOpts[] =
{
	InterfaceOption(),
	StringOption( 'n', "name", &gQueryRecord_Name, "domain name", "Full domain name of record to query.", true ),
	StringOption( 't', "type", &gQueryRecord_Type, "record type", "Record type by name (e.g., TXT, SRV, etc.) or number.", true ),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption(),
	DNSSDFlagsOption_IncludeAWDL(),
	DNSSDFlagsOption_ForceMulticast(),
	DNSSDFlagsOption_Timeout(),
	DNSSDFlagsOption_ReturnIntermediates(),
	DNSSDFlagsOption_SuppressUnusable(),
	DNSSDFlagsOption_UnicastResponse(),
	DNSSDFlagsOption_DenyCellular(),
	DNSSDFlagsOption_DenyExpensive(),
	DNSSDFlagsOption_PathEvalDone(),
	
	CLI_OPTION_GROUP( "Operation" ),
	ConnectionOptions(),
	BooleanOption( 'o', "oneshot",		&gQueryRecord_OneShot,			"Finish after first set of results." ),
	IntegerOption( 'l', "timeLimit",	&gQueryRecord_TimeLimitSecs,	"seconds", "Maximum duration of the query record operation. Use '0' for no time limit.", false ),
	BooleanOption( 'r', "raw",			&gQueryRecord_RawRData,			"Show record data as a hexdump." ),
	
	ConnectionSection(),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	Register Command Options
//===========================================================================================================================

static const char *			gRegister_Name			= NULL;
static const char *			gRegister_Type			= NULL;
static const char *			gRegister_Domain		= NULL;
static int					gRegister_Port			= 0;
static const char *			gRegister_TXT			= NULL;
static int					gRegister_LifetimeMs	= -1;
static const char **		gAddRecord_Types		= NULL;
static size_t				gAddRecord_TypesCount	= 0;
static const char **		gAddRecord_Data			= NULL;
static size_t				gAddRecord_DataCount	= 0;
static const char **		gAddRecord_TTLs			= NULL;
static size_t				gAddRecord_TTLsCount	= 0;
static const char *			gUpdateRecord_Data		= NULL;
static int					gUpdateRecord_DelayMs	= 0;
static int					gUpdateRecord_TTL		= 0;

static CLIOption		kRegisterOpts[] =
{
	InterfaceOption(),
	StringOption(  'n', "name",		&gRegister_Name,	"service name",	"Name of service.", false ),
	StringOption(  't', "type",		&gRegister_Type,	"service type",	"Service type, e.g., \"_ssh._tcp\".", true ),
	StringOption(  'd', "domain",	&gRegister_Domain,	"domain",		"Domain in which to advertise the service.", false ),
	IntegerOption( 'p', "port",		&gRegister_Port,	"port number",	"Service's port number.", true ),
	StringOption(   0 , "txt",		&gRegister_TXT,		"record data",	"The TXT record data. See " kRecordDataSection_Name " below.", false ),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption(),
	DNSSDFlagsOption_IncludeAWDL(),
	DNSSDFlagsOption_NoAutoRename(),
	
	CLI_OPTION_GROUP( "Operation" ),
	IntegerOption( 'l', "lifetime", &gRegister_LifetimeMs, "ms", "Lifetime of the service registration in milliseconds.", false ),
	
	CLI_OPTION_GROUP( "Options for updating the registered service's primary TXT record with DNSServiceUpdateRecord()\n" ),
	StringOption(  0 , "updateData",	&gUpdateRecord_Data,	"record data",	"Record data for the record update. See " kRecordDataSection_Name " below.", false ),
	IntegerOption( 0 , "updateDelay",	&gUpdateRecord_DelayMs,	"ms",			"Number of milliseconds after registration to wait before record update.", false ),
	IntegerOption( 0 , "updateTTL",		&gUpdateRecord_TTL,		"seconds",		"Time-to-live of the updated record.", false ),
	
	CLI_OPTION_GROUP( "Options for adding extra record(s) to the registered service with DNSServiceAddRecord()\n" ),
	MultiStringOption(   0 , "addType",	&gAddRecord_Types,	&gAddRecord_TypesCount,	"record type",	"Type of additional record by name (e.g., TXT, SRV, etc.) or number.", false ),
	MultiStringOptionEx( 0 , "addData",	&gAddRecord_Data,	&gAddRecord_DataCount,	"record data",	"Additional record's data. See " kRecordDataSection_Name " below.", false, NULL ),
	MultiStringOption(   0 , "addTTL",	&gAddRecord_TTLs,	&gAddRecord_TTLsCount,	"seconds",		"Time-to-live of additional record in seconds. Use '0' for default.", false ),
	
	RecordDataSection(),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	RegisterRecord Command Options
//===========================================================================================================================

static const char *		gRegisterRecord_Name			= NULL;
static const char *		gRegisterRecord_Type			= NULL;
static const char *		gRegisterRecord_Data			= NULL;
static int				gRegisterRecord_TTL				= 0;
static int				gRegisterRecord_LifetimeMs		= -1;
static const char *		gRegisterRecord_UpdateData		= NULL;
static int				gRegisterRecord_UpdateDelayMs	= 0;
static int				gRegisterRecord_UpdateTTL		= 0;

static CLIOption		kRegisterRecordOpts[] =
{
	InterfaceOption(),
	StringOption( 'n', "name",	&gRegisterRecord_Name,	"record name",	"Fully qualified domain name of record.", true ),
	StringOption( 't', "type",	&gRegisterRecord_Type,	"record type",	"Record type by name (e.g., TXT, PTR, A) or number.", true ),
	StringOption( 'd', "data",	&gRegisterRecord_Data,	"record data",	"The record data. See " kRecordDataSection_Name " below.", false ),
	IntegerOption( 0 , "ttl",	&gRegisterRecord_TTL,	"seconds",		"Time-to-live in seconds. Use '0' for default.", false ),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption(),
	DNSSDFlagsOption_IncludeAWDL(),
	DNSSDFlagsOption_Shared(),
	DNSSDFlagsOption_Unique(),
	
	CLI_OPTION_GROUP( "Operation" ),
	IntegerOption( 'l', "lifetime", &gRegisterRecord_LifetimeMs, "ms", "Lifetime of the service registration in milliseconds.", false ),
	
	CLI_OPTION_GROUP( "Options for updating the registered record with DNSServiceUpdateRecord()\n" ),
	StringOption(  0 , "updateData",	&gRegisterRecord_UpdateData,	"record data",	"Record data for the record update.", false ),
	IntegerOption( 0 , "updateDelay",	&gRegisterRecord_UpdateDelayMs,	"ms",			"Number of milliseconds after registration to wait before record update.", false ),
	IntegerOption( 0 , "updateTTL",		&gRegisterRecord_UpdateTTL,		"seconds",		"Time-to-live of the updated record.", false ),
	
	RecordDataSection(),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	Resolve Command Options
//===========================================================================================================================

static char *		gResolve_Name			= NULL;
static char *		gResolve_Type			= NULL;
static char *		gResolve_Domain			= NULL;
static int			gResolve_TimeLimitSecs	= 0;

static CLIOption		kResolveOpts[] =
{
	InterfaceOption(),
	StringOption( 'n', "name",		&gResolve_Name,		"service name", "Name of the service instance to resolve.", true ),
	StringOption( 't', "type",		&gResolve_Type,		"service type", "Type of the service instance to resolve.", true ),
	StringOption( 'd', "domain",	&gResolve_Domain,	"domain", "Domain of the service instance to resolve.", true ),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption(),
	DNSSDFlagsOption_ForceMulticast(),
	DNSSDFlagsOption_IncludeAWDL(),
	DNSSDFlagsOption_ReturnIntermediates(),
	
	CLI_OPTION_GROUP( "Operation" ),
	ConnectionOptions(),
	IntegerOption( 'l', "timeLimit", &gResolve_TimeLimitSecs, "seconds", "Maximum duration of the resolve operation. Use '0' for no time limit.", false ),
	
	ConnectionSection(),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	Reconfirm Command Options
//===========================================================================================================================

static const char *		gReconfirmRecord_Name	= NULL;
static const char *		gReconfirmRecord_Type	= NULL;
static const char *		gReconfirmRecord_Class	= NULL;
static const char *		gReconfirmRecord_Data	= NULL;

static CLIOption		kReconfirmOpts[] =
{
	InterfaceOption(),
	StringOption( 'n', "name",	&gReconfirmRecord_Name,		"record name",	"Full name of the record to reconfirm.", true ),
	StringOption( 't', "type",	&gReconfirmRecord_Type,		"record type",	"Type of the record to reconfirm.", true ),
	StringOption( 'c', "class",	&gReconfirmRecord_Class,	"record class",	"Class of the record to reconfirm. Default class is IN.", false ),
	StringOption( 'd', "data",	&gReconfirmRecord_Data,		"record data",	"The record data. See " kRecordDataSection_Name " below.", false ),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption(),
	
	RecordDataSection(),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	getaddrinfo-POSIX Command Options
//===========================================================================================================================

static const char *		gGAIPOSIX_Name		= NULL;
static const char *		gGAIPOSIX_Family	= NULL;

static CLIOption		kGetAddrInfoPOSIXOpts[] =
{
	StringOption(	'n', "name",	&gGAIPOSIX_Name,	"domain name", "Domain name to resolve.", true ),
	StringOptionEx(	'f', "family",	&gGAIPOSIX_Family,	"address family", "Address family to pass to getaddrinfo().", false,
		"The address family value determines which value to use for the ai_family element of the hints struct addrinfo\n"
		"argument used when calling getaddrinfo().\n"
		"\n"
		"Possible address family values are 'inet' for AF_INET, 'inet6' for AF_INET6, or 'unspec' for AF_UNSPEC. If no\n"
		"address family is specified, then AF_UNSPEC is used.\n" ),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	ReverseLookup Command Options
//===========================================================================================================================

static const char *		gReverseLookup_IPAddr			= NULL;
static int				gReverseLookup_OneShot			= false;
static int				gReverseLookup_TimeLimitSecs	= 0;

static CLIOption		kReverseLookupOpts[] =
{
	InterfaceOption(),
	StringOption( 'a', "address", &gReverseLookup_IPAddr, "IP address", "IPv4 or IPv6 address for which to perform a reverse IP lookup.", true ),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption(),
	DNSSDFlagsOption_ForceMulticast(),
	DNSSDFlagsOption_ReturnIntermediates(),
	DNSSDFlagsOption_SuppressUnusable(),
	
	CLI_OPTION_GROUP( "Operation" ),
	ConnectionOptions(),
	BooleanOption( 'o', "oneshot",		&gReverseLookup_OneShot,		"Finish after first set of results." ),
	IntegerOption( 'l', "timeLimit",	&gReverseLookup_TimeLimitSecs,	"seconds", "Specifies the max duration of the query record operation. Use '0' for no time limit.", false ),
	
	ConnectionSection(),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	BrowseAll Command Options
//===========================================================================================================================

static const char *		gBrowseAll_Domain				= NULL;
static char **			gBrowseAll_ServiceTypes			= NULL;
static size_t			gBrowseAll_ServiceTypesCount	= 0;
static int				gBrowseAll_IncludeAWDL			= false;
static int				gBrowseAll_BrowseTimeSecs		= 5;
static int				gBrowseAll_ConnectTimeLimitSecs	= 5;

static CLIOption		kBrowseAllOpts[] =
{
	InterfaceOption(),
	StringOption(	   'd', "domain",	&gBrowseAll_Domain, "domain", "Domain in which to browse for the service.", false ),
	MultiStringOption( 't', "type",		&gBrowseAll_ServiceTypes, &gBrowseAll_ServiceTypesCount, "service type", "Service type(s), e.g., \"_ssh._tcp\".", false ),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption_IncludeAWDL(),
	
	CLI_OPTION_GROUP( "Operation" ),
	IntegerOption( 'b', "browseTime",		&gBrowseAll_BrowseTimeSecs,			"seconds", "Specifies the duration of the browse.", false ),
	IntegerOption( 'c', "connectTimeLimit",	&gBrowseAll_ConnectTimeLimitSecs,	"seconds", "Specifies the max duration of the connect operations.", false ),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	GetAddrInfoStress Command Options
//===========================================================================================================================

static int		gGAIStress_TestDurationSecs	= 0;
static int		gGAIStress_ConnectionCount	= 0;
static int		gGAIStress_DurationMinMs	= 0;
static int		gGAIStress_DurationMaxMs	= 0;
static int		gGAIStress_RequestCountMax	= 0;

static CLIOption		kGetAddrInfoStressOpts[] =
{
	InterfaceOption(),
	
	CLI_OPTION_GROUP( "Flags" ),
	DNSSDFlagsOption_ReturnIntermediates(),
	DNSSDFlagsOption_SuppressUnusable(),
	
	CLI_OPTION_GROUP( "Operation" ),
	IntegerOption( 0, "testDuration",			&gGAIStress_TestDurationSecs,	"seconds",	"Stress test duration in seconds. Use '0' for forever.", false ),
	IntegerOption( 0, "connectionCount",		&gGAIStress_ConnectionCount,	"integer",	"Number of simultaneous DNS-SD connections.", true ),
	IntegerOption( 0, "requestDurationMin",		&gGAIStress_DurationMinMs,		"ms",		"Minimum duration of DNSServiceGetAddrInfo() request in milliseconds.", true ),
	IntegerOption( 0, "requestDurationMax",		&gGAIStress_DurationMaxMs,		"ms",		"Maximum duration of DNSServiceGetAddrInfo() request in milliseconds.", true ),
	IntegerOption( 0, "consecutiveRequestMax",	&gGAIStress_RequestCountMax,	"integer",	"Maximum number of requests on a connection before restarting it.", true ),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	DNSQuery Command Options
//===========================================================================================================================

static char *		gDNSQuery_Name			= NULL;
static char *		gDNSQuery_Type			= "A";
static char *		gDNSQuery_Server		= NULL;
static int			gDNSQuery_TimeLimitSecs	= 5;
static int			gDNSQuery_UseTCP		= false;
static int			gDNSQuery_Flags			= kDNSHeaderFlag_RecursionDesired;
static int			gDNSQuery_RawRData		= false;
static int			gDNSQuery_Verbose		= false;

static CLIOption		kDNSQueryOpts[] =
{
	StringOption(  'n', "name",			&gDNSQuery_Name,			"name",	"Question name (QNAME) to put in DNS query message.", true ),
	StringOption(  't', "type",			&gDNSQuery_Type,			"type",	"Question type (QTYPE) to put in DNS query message. Default value is 'A'.", false ),
	StringOption(  's', "server",		&gDNSQuery_Server,			"IP address", "DNS server's IPv4 or IPv6 address.", true ),
	IntegerOption( 'l', "timeLimit",	&gDNSQuery_TimeLimitSecs,	"seconds", "Specifies query time limit. Use '-1' for no limit and '0' to exit immediately after sending.", false ),
	BooleanOption(  0 , "tcp",			&gDNSQuery_UseTCP,			"Send the DNS query via TCP instead of UDP." ),
	IntegerOption( 'f', "flags",		&gDNSQuery_Flags,			"flags", "16-bit value for DNS header flags/codes field. Default value is 0x0100 (Recursion Desired).", false ),
	BooleanOption( 'r', "raw",			&gDNSQuery_RawRData,		"Present record data as a hexdump." ),
	BooleanOption( 'v', "verbose",		&gDNSQuery_Verbose,			"Prints the DNS message to be sent to the server." ),
	CLI_OPTION_END()
};

#if( DNSSDUTIL_INCLUDE_DNSCRYPT )
//===========================================================================================================================
//	DNSCrypt Command Options
//===========================================================================================================================

static char *		gDNSCrypt_ProviderName	= NULL;
static char *		gDNSCrypt_ProviderKey	= NULL;
static char *		gDNSCrypt_Name			= NULL;
static char *		gDNSCrypt_Type			= NULL;
static char *		gDNSCrypt_Server		= NULL;
static int			gDNSCrypt_TimeLimitSecs	= 5;
static int			gDNSCrypt_RawRData		= false;
static int			gDNSCrypt_Verbose		= false;

static CLIOption		kDNSCryptOpts[] =
{
	StringOption(  'p', "providerName",	&gDNSCrypt_ProviderName,	"name", "The DNSCrypt provider name.", true ),
	StringOption(  'k', "providerKey",	&gDNSCrypt_ProviderKey,		"hex string", "The DNSCrypt provider's public signing key.", true ),
	StringOption(  'n', "name",			&gDNSCrypt_Name,			"name",	"Question name (QNAME) to put in DNS query message.", true ),
	StringOption(  't', "type",			&gDNSCrypt_Type,			"type",	"Question type (QTYPE) to put in DNS query message.", true ),
	StringOption(  's', "server",		&gDNSCrypt_Server,			"IP address", "DNS server's IPv4 or IPv6 address.", true ),
	IntegerOption( 'l', "timeLimit",	&gDNSCrypt_TimeLimitSecs,	"seconds", "Specifies query time limit. Use '-1' for no time limit and '0' to exit immediately after sending.", false ),
	BooleanOption( 'r', "raw",			&gDNSCrypt_RawRData,		"Present record data as a hexdump." ),
	BooleanOption( 'v', "verbose",		&gDNSCrypt_Verbose,			"Prints the DNS message to be sent to the server." ),
	CLI_OPTION_END()
};
#endif

//===========================================================================================================================
//	MDNSQuery Command Options
//===========================================================================================================================

static char *		gMDNSQuery_Name			= NULL;
static char *		gMDNSQuery_Type			= NULL;
static int			gMDNSQuery_SourcePort	= 0;
static int			gMDNSQuery_IsQU			= false;
static int			gMDNSQuery_RawRData		= false;
static int			gMDNSQuery_UseIPv4		= false;
static int			gMDNSQuery_UseIPv6		= false;
static int			gMDNSQuery_AllResponses	= false;

static CLIOption		kMDNSQueryOpts[] =
{
	StringOption(  'i', "interface",	&gInterface,				"name or index", "Network interface by name or index.", true ),
	StringOption(  'n', "name",			&gMDNSQuery_Name,			"name", "Question name (QNAME) to put in mDNS message.", true ),
	StringOption(  't', "type",			&gMDNSQuery_Type,			"type", "Question type (QTYPE) to put in mDNS message.", true ),
	IntegerOption( 'p', "sourcePort",	&gMDNSQuery_SourcePort,		"port number", "UDP source port to use when sending mDNS messages. Default is 5353 for QM questions.", false ),
	BooleanOption( 'u', "QU",			&gMDNSQuery_IsQU,			"Set the unicast-response bit, i.e., send a QU question." ),
	BooleanOption( 'r', "raw",			&gMDNSQuery_RawRData,		"Present record data as a hexdump." ),
	BooleanOption(  0 , "ipv4",			&gMDNSQuery_UseIPv4,		"Use IPv4." ),
	BooleanOption(  0 , "ipv6",			&gMDNSQuery_UseIPv6,		"Use IPv6." ),
	BooleanOption( 'a', "allResponses",	&gMDNSQuery_AllResponses,	"Print all responses." ),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	PIDToUUID Command Options
//===========================================================================================================================

static int		gPIDToUUID_PID = 0;

static CLIOption		kPIDToUUIDOpts[] =
{
	IntegerOption( 'p', "pid", &gPIDToUUID_PID, "PID", "Process ID.", true ),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	Command Table
//===========================================================================================================================

static OSStatus	VersionOptionCallback( CLIOption *inOption, const char *inArg, int inUnset );

static void	BrowseCmd( void );
static void	GetAddrInfoCmd( void );
static void	QueryRecordCmd( void );
static void	RegisterCmd( void );
static void	RegisterRecordCmd( void );
static void	ResolveCmd( void );
static void	ReconfirmCmd( void );
static void	GetAddrInfoPOSIXCmd( void );
static void	ReverseLookupCmd( void );
static void	BrowseAllCmd( void );
static void	GetAddrInfoStressCmd( void );
static void	DNSQueryCmd( void );
#if( DNSSDUTIL_INCLUDE_DNSCRYPT )
static void	DNSCryptCmd( void );
#endif
static void	MDNSQueryCmd( void );
static void	PIDToUUIDCmd( void );
static void	DaemonVersionCmd( void );

static CLIOption		kGlobalOpts[] =
{
	CLI_OPTION_CALLBACK_EX( 'V', "version", VersionOptionCallback, NULL, NULL,
		kCLIOptionFlags_NoArgument | kCLIOptionFlags_GlobalOnly, "Displays the version of this tool.", NULL ),
	CLI_OPTION_HELP(),
	
	// Common commands.
	
	Command( "browse",				BrowseCmd,				kBrowseOpts,			"Uses DNSServiceBrowse() to browse for one or more service types.", false ),
	Command( "getAddrInfo",			GetAddrInfoCmd,			kGetAddrInfoOpts,		"Uses DNSServiceGetAddrInfo() to resolve a hostname to IP addresses.", false ),
	Command( "queryRecord",			QueryRecordCmd,			kQueryRecordOpts,		"Uses DNSServiceQueryRecord() to query for an arbitrary DNS record.", false ),
	Command( "register",			RegisterCmd,			kRegisterOpts,			"Uses DNSServiceRegister() to register a service.", false ),
	Command( "registerRecord",		RegisterRecordCmd,		kRegisterRecordOpts,	"Uses DNSServiceRegisterRecord() to register a record.", false ),
	Command( "resolve",				ResolveCmd,				kResolveOpts,			"Uses DNSServiceResolve() to resolve a service.", false ),
	Command( "reconfirm",			ReconfirmCmd,			kReconfirmOpts,			"Uses DNSServiceReconfirmRecord() to reconfirm a record.", false ),
	Command( "getaddrinfo-posix",	GetAddrInfoPOSIXCmd,	kGetAddrInfoPOSIXOpts,	"Uses getaddrinfo() to resolve a hostname to IP addresses.", false ),
	Command( "reverseLookup",		ReverseLookupCmd,		kReverseLookupOpts,		"Uses DNSServiceQueryRecord() to perform a reverse IP address lookup.", false ),
	Command( "browseAll",			BrowseAllCmd,			kBrowseAllOpts,			"Browse and resolve all, or just some, services.", false ),
	
	// Uncommon commands.
	
	Command( "getAddrInfoStress",	GetAddrInfoStressCmd,	kGetAddrInfoStressOpts,	"Runs DNSServiceGetAddrInfo() stress testing.", true ),
	Command( "DNSQuery",			DNSQueryCmd,			kDNSQueryOpts,			"Crafts and sends a DNS query.", true ),
#if( DNSSDUTIL_INCLUDE_DNSCRYPT )
	Command( "DNSCrypt",			DNSCryptCmd,			kDNSCryptOpts,			"Crafts and sends a DNSCrypt query.", true ),
#endif
	Command( "mDNSQuery",			MDNSQueryCmd,			kMDNSQueryOpts,			"Crafts and sends an mDNS query over the specified interface.", true ),
	Command( "pid2uuid",			PIDToUUIDCmd,			kPIDToUUIDOpts,			"Prints the UUID of a process.", true ),
	Command( "daemonVersion",		DaemonVersionCmd,		NULL,					"Prints the version of the DNS-SD daemon.", true ),
	
	CLI_COMMAND_HELP(),
	CLI_OPTION_END()
};

//===========================================================================================================================
//	Helper Prototypes
//===========================================================================================================================

#define kExitReason_OneShotDone				"one-shot done"
#define kExitReason_ReceivedResponse		"received response"
#define kExitReason_SIGINT					"interrupt signal"
#define kExitReason_Timeout					"timeout"
#define kExitReason_TimeLimit				"time limit"

static void	Exit( void *inContext ) ATTRIBUTE_NORETURN;

#define kTimestampBufLen		27

static char *			GetTimestampStr( char inBuffer[ kTimestampBufLen ] );
static DNSServiceFlags	GetDNSSDFlagsFromOpts( void );

typedef enum
{
	kConnectionType_None			= 0,
	kConnectionType_Normal			= 1,
	kConnectionType_DelegatePID		= 2,
	kConnectionType_DelegateUUID	= 3
	
}	ConnectionType;

typedef struct
{
	ConnectionType		type;
	union
	{
		int32_t			pid;
		uint8_t			uuid[ 16 ];
		
	}	delegate;
	
}	ConnectionDesc;

static OSStatus
	CreateConnectionFromArgString(
		const char *			inString,
		dispatch_queue_t		inQueue,
		DNSServiceRef *			outSDRef,
		ConnectionDesc *		outDesc );
static OSStatus			InterfaceIndexFromArgString( const char *inString, uint32_t *outIndex );
static OSStatus			RecordDataFromArgString( const char *inString, uint8_t **outDataPtr, size_t *outDataLen );
static OSStatus			RecordTypeFromArgString( const char *inString, uint16_t *outValue );
static OSStatus			RecordClassFromArgString( const char *inString, uint16_t *outValue );

#define kInterfaceNameBufLen		( Max( IF_NAMESIZE, 16 ) + 1 )

static char *			InterfaceIndexToName( uint32_t inIfIndex, char inNameBuf[ kInterfaceNameBufLen ] );
static const char *		RecordTypeToString( unsigned int inValue );

// DNS message helpers

typedef struct
{
	uint8_t		id[ 2 ];
	uint8_t		flags[ 2 ];
	uint8_t		questionCount[ 2 ];
	uint8_t		answerCount[ 2 ];
	uint8_t		authorityCount[ 2 ];
	uint8_t		additionalCount[ 2 ];
	
}	DNSHeader;

#define kDNSHeaderLength		12
check_compile_time( sizeof( DNSHeader ) == kDNSHeaderLength );

#define DNSHeaderGetID( HDR )					ReadBig16( ( HDR )->id )
#define DNSHeaderGetFlags( HDR )				ReadBig16( ( HDR )->flags )
#define DNSHeaderGetQuestionCount( HDR )		ReadBig16( ( HDR )->questionCount )
#define DNSHeaderGetAnswerCount( HDR )			ReadBig16( ( HDR )->answerCount )
#define DNSHeaderGetAuthorityCount( HDR )		ReadBig16( ( HDR )->authorityCount )
#define DNSHeaderGetAdditionalCount( HDR )		ReadBig16( ( HDR )->additionalCount )

static OSStatus
	DNSMessageExtractDomainName(
		const uint8_t *		inMsgPtr,
		size_t				inMsgLen,
		const uint8_t *		inNamePtr,
		uint8_t				inBuf[ kDomainNameLengthMax ],
		const uint8_t **	outNextPtr );
static OSStatus
	DNSMessageExtractDomainNameString(
		const void *		inMsgPtr,
		size_t				inMsgLen,
		const void *		inNamePtr,
		char				inBuf[ kDNSServiceMaxDomainName ],
		const uint8_t **	outNextPtr );
static OSStatus
	DNSMessageExtractRecord(
		const uint8_t *		inMsgPtr,
		size_t				inMsgLen,
		const uint8_t *		inPtr,
		uint8_t				inNameBuf[ kDomainNameLengthMax ],
		uint16_t *			outType,
		uint16_t *			outClass,
		uint32_t *			outTTL,
		const uint8_t **	outRDataPtr,
		size_t *			outRDataLen,
		const uint8_t **	outPtr );
static OSStatus	DNSMessageGetAnswerSection( const uint8_t *inMsgPtr, size_t inMsgLen, const uint8_t **outPtr );
static OSStatus
	DNSRecordDataToString(
		const void *	inRDataPtr,
		size_t			inRDataLen,
		unsigned int	inRDataType,
		const void *	inMsgPtr,
		size_t			inMsgLen,
		char **			outString );
static OSStatus
	DomainNameAppendString(
		uint8_t			inDomainName[ kDomainNameLengthMax ],
		const char *	inString,
		uint8_t **		outEndPtr );
static Boolean	DomainNameEqual( const uint8_t *inName1, const uint8_t *inName2 );
static OSStatus
	DomainNameToString(
		const uint8_t *		inDomainName,
		const uint8_t *		inEnd,
		char				inBuf[ kDNSServiceMaxDomainName ],
		const uint8_t **	outNextPtr );

static OSStatus	PrintDNSMessage( const uint8_t *inMsgPtr, size_t inMsgLen, Boolean inIsMDNS, Boolean inPrintRaw );

#define PrintMDNSMessage( MSGPTR, MSGLEN, RAW )		PrintDNSMessage( MSGPTR, MSGLEN, true, RAW )
#define PrintUDNSMessage( MSGPTR, MSGLEN, RAW )		PrintDNSMessage( MSGPTR, MSGLEN, false, RAW )

#define kDNSQueryMessageMaxLen		( kDNSHeaderLength + kDomainNameLengthMax + 4 )

static OSStatus
	WriteDNSQueryMessage(
		uint8_t			inMsg[ kDNSQueryMessageMaxLen ],
		uint16_t		inMsgID,
		uint16_t		inFlags,
		const char *	inQName,
		uint16_t		inQType,
		uint16_t		inQClass,
		size_t *		outMsgLen );

// Dispatch helpers

typedef void ( *DispatchHandler )( void *inContext );

static OSStatus
	DispatchSignalSourceCreate(
		int					inSignal,
		DispatchHandler		inEventHandler,
		void *				inContext,
		dispatch_source_t *	outSource );
static OSStatus
	DispatchReadSourceCreate(
		SocketRef			inSock,
		DispatchHandler		inEventHandler,
		DispatchHandler		inCancelHandler,
		void *				inContext,
		dispatch_source_t *	outSource );
static OSStatus
	DispatchTimerCreate(
		dispatch_time_t		inStart,
		uint64_t			inIntervalNs,
		uint64_t			inLeewayNs,
		DispatchHandler		inEventHandler,
		DispatchHandler		inCancelHandler,
		void *				inContext,
		dispatch_source_t *	outTimer );

static const char *	ServiceTypeDescription( const char *inName );

typedef struct
{
	SocketRef		sock;
	void *			context;
	
}	SocketContext;

static void			SocketContextCancelHandler( void *inContext );
static OSStatus		StringToInt32( const char *inString, int32_t *outValue );
static OSStatus		StringToUInt32( const char *inString, uint32_t *outValue );

#define AddRmvString( X )		( ( (X) & kDNSServiceFlagsAdd ) ? "Add" : "Rmv" )
#define Unused( X )				(void)(X)

//===========================================================================================================================
//	main
//===========================================================================================================================

int	main( int argc, const char **argv )
{
	// Route DebugServices logging output to stderr.
	
	dlog_control( "DebugServices:output=file;stderr" );
	
	CLIInit( argc, argv );
	CLIParse( kGlobalOpts, kCLIFlags_None );
	
	return( gExitCode );
}

//===========================================================================================================================
//	VersionOptionCallback
//===========================================================================================================================

static OSStatus	VersionOptionCallback( CLIOption *inOption, const char *inArg, int inUnset )
{
	const char *		srcVers;
#if( MDNSRESPONDER_PROJECT )
	char				srcStr[ 16 ];
#endif
	
	Unused( inOption );
	Unused( inArg );
	Unused( inUnset );
	
#if( MDNSRESPONDER_PROJECT )
	srcVers = SourceVersionToCString( _DNS_SD_H, srcStr );
#else
	srcVers = DNSSDUTIL_SOURCE_VERSION;
#endif
	FPrintF( stdout, "%s version %v (%s)\n", gProgramName, kDNSSDUtilNumVersion, srcVers );
	
	return( kEndingErr );
}

//===========================================================================================================================
//	BrowseCmd
//===========================================================================================================================

typedef struct BrowseResolveOp		BrowseResolveOp;

struct BrowseResolveOp
{
	BrowseResolveOp *		next;			// Next resolve operation in list.
	DNSServiceRef			sdRef;			// sdRef of the DNSServiceResolve or DNSServiceQueryRecord operation.
	char *					fullName;		// Full name of the service to resolve.
	uint32_t				interfaceIndex;	// Interface index of the DNSServiceResolve or DNSServiceQueryRecord operation.
};

typedef struct
{
	DNSServiceRef			mainRef;			// Main sdRef for shared connection.
	DNSServiceRef *			opRefs;				// Array of sdRefs for individual Browse operarions.
	size_t					opRefsCount;		// Count of array of sdRefs for non-shared connections.
	const char *			domain;				// Domain for DNSServiceBrowse operation(s).
	DNSServiceFlags			flags;				// Flags for DNSServiceBrowse operation(s).
	char **					serviceTypes;		// Array of service types to browse for.
	size_t					serviceTypesCount;	// Count of array of service types to browse for.
	int						timeLimitSecs;		// Time limit of DNSServiceBrowse operation in seconds.
	BrowseResolveOp *		resolveList;		// List of resolve and/or TXT record query operations.
	uint32_t				ifIndex;			// Interface index of DNSServiceBrowse operation(s).
	Boolean					printedHeader;		// True if results header has been printed.
	Boolean					doResolve;			// True if service instances are to be resolved.
	Boolean					doResolveTXTOnly;	// True if TXT records of service instances are to be queried.
	
}	BrowseContext;

static void		BrowsePrintPrologue( const BrowseContext *inContext );
static void		BrowseContextFree( BrowseContext *inContext );
static OSStatus	BrowseResolveOpCreate( const char *inFullName, uint32_t inInterfaceIndex, BrowseResolveOp **outOp );
static void		BrowseResolveOpFree( BrowseResolveOp *inOp );
static void DNSSD_API
	BrowseCallback(
		DNSServiceRef		inSDRef,
		DNSServiceFlags		inFlags,
		uint32_t			inInterfaceIndex,
		DNSServiceErrorType	inError,
		const char *		inName,
		const char *		inRegType,
		const char *		inDomain,
		void *				inContext );
static void DNSSD_API
	BrowseResolveCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		const char *			inHostname,
		uint16_t				inPort,
		uint16_t				inTXTLen,
		const unsigned char *	inTXTPtr,
		void *					inContext );
static void DNSSD_API
	BrowseQueryRecordCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		uint16_t				inType,
		uint16_t				inClass,
		uint16_t				inRDataLen,
		const void *			inRDataPtr,
		uint32_t				inTTL,
		void *					inContext );

static void	BrowseCmd( void )
{
	OSStatus				err;
	size_t					i;
	BrowseContext *			context			= NULL;
	dispatch_source_t		signalSource	= NULL;
	int						useMainConnection;
	
	// Set up SIGINT handler.
	
	signal( SIGINT, SIG_IGN );
	err = DispatchSignalSourceCreate( SIGINT, Exit, kExitReason_SIGINT, &signalSource );
	require_noerr( err, exit );
	dispatch_resume( signalSource );
	
	// Create context.
	
	context = (BrowseContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	context->opRefs = (DNSServiceRef *) calloc( gBrowse_ServiceTypesCount, sizeof( DNSServiceRef ) );
	require_action( context->opRefs, exit, err = kNoMemoryErr );
	context->opRefsCount = gBrowse_ServiceTypesCount;
	
	// Check command parameters.
	
	if( gBrowse_TimeLimitSecs < 0 )
	{
		FPrintF( stderr, "Invalid time limit: %d seconds.\n", gBrowse_TimeLimitSecs );
		err = kParamErr;
		goto exit;
	}
	
	// Create main connection.
	
	if( gConnectionOpt )
	{
		err = CreateConnectionFromArgString( gConnectionOpt, dispatch_get_main_queue(), &context->mainRef, NULL );
		require_noerr_quiet( err, exit );
		useMainConnection = true;
	}
	else
	{
		useMainConnection = false;
	}
	
	// Get flags.
	
	context->flags = GetDNSSDFlagsFromOpts();
	if( useMainConnection ) context->flags |= kDNSServiceFlagsShareConnection;
	
	// Get interface.
	
	err = InterfaceIndexFromArgString( gInterface, &context->ifIndex );
	require_noerr_quiet( err, exit );
	
	// Set remaining parameters.
	
	context->serviceTypes		= gBrowse_ServiceTypes;
	context->serviceTypesCount	= gBrowse_ServiceTypesCount;
	context->domain				= gBrowse_Domain;
	context->doResolve			= gBrowse_DoResolve	? true : false;
	context->timeLimitSecs		= gBrowse_TimeLimitSecs;
	context->doResolveTXTOnly	= gBrowse_QueryTXT	? true : false;
	
	// Print prologue.
	
	BrowsePrintPrologue( context );
	
	// Start operation(s).
	
	for( i = 0; i < context->serviceTypesCount; ++i )
	{
		DNSServiceRef		sdRef;
		
		if( useMainConnection ) sdRef = context->mainRef;
		err = DNSServiceBrowse( &sdRef, context->flags, context->ifIndex, context->serviceTypes[ i ], context->domain,
			BrowseCallback, context );
		require_noerr( err, exit );
		
		context->opRefs[ i ] = sdRef;
		if( !useMainConnection )
		{
			err = DNSServiceSetDispatchQueue( context->opRefs[ i ], dispatch_get_main_queue() );
			require_noerr( err, exit );
		}
	}
	
	// Set time limit.
	
	if( context->timeLimitSecs > 0 )
	{
		dispatch_after_f( dispatch_time_seconds( context->timeLimitSecs ), dispatch_get_main_queue(),
			kExitReason_TimeLimit, Exit );
	}
	dispatch_main();
	
exit:
	dispatch_source_forget( &signalSource );
	if( context ) BrowseContextFree( context );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	BrowsePrintPrologue
//===========================================================================================================================

static void	BrowsePrintPrologue( const BrowseContext *inContext )
{
	const int						timeLimitSecs	= inContext->timeLimitSecs;
	const char * const *			serviceType		= (const char **) inContext->serviceTypes;
	const char * const * const		end				= (const char **) inContext->serviceTypes + inContext->serviceTypesCount;
	char							time[ kTimestampBufLen ];
	char							ifName[ kInterfaceNameBufLen ];
	
	InterfaceIndexToName( inContext->ifIndex, ifName );
	
	FPrintF( stdout, "Flags:         %#{flags}\n",	inContext->flags, kDNSServiceFlagsDescriptors );
	FPrintF( stdout, "Interface:     %d (%s)\n",	(int32_t) inContext->ifIndex, ifName );
	FPrintF( stdout, "Service types: %s",			*serviceType++ );
	while( serviceType < end ) FPrintF( stdout, ", %s", *serviceType++ );
	FPrintF( stdout, "\n" );
	FPrintF( stdout, "Domain:        %s\n",	inContext->domain ? inContext->domain : "<NULL> (default domains)" );
	FPrintF( stdout, "Time limit:    " );
	if( timeLimitSecs > 0 )	FPrintF( stdout, "%d second%?c\n", timeLimitSecs, timeLimitSecs != 1, 's' );
	else					FPrintF( stdout, "∞\n" );
	FPrintF( stdout, "Start time:    %s\n", GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
}

//===========================================================================================================================
//	BrowseContextFree
//===========================================================================================================================

static void	BrowseContextFree( BrowseContext *inContext )
{
	size_t		i;
	
	for( i = 0; i < inContext->opRefsCount; ++i )
	{
		DNSServiceForget( &inContext->opRefs[ i ] );
	}
	if( inContext->serviceTypes )
	{
		StringArray_Free( inContext->serviceTypes, inContext->serviceTypesCount );
		inContext->serviceTypes			= NULL;
		inContext->serviceTypesCount	= 0;
	}
	DNSServiceForget( &inContext->mainRef );
	free( inContext );
}

//===========================================================================================================================
//	BrowseResolveOpCreate
//===========================================================================================================================

static OSStatus	BrowseResolveOpCreate( const char *inFullName, uint32_t inInterfaceIndex, BrowseResolveOp **outOp )
{
	OSStatus				err;
	BrowseResolveOp *		resolveOp;
	
	resolveOp = (BrowseResolveOp *) calloc( 1, sizeof( *resolveOp ) );
	require_action( resolveOp, exit, err = kNoMemoryErr );
	
	resolveOp->fullName = strdup( inFullName );
	require_action( resolveOp->fullName, exit, err = kNoMemoryErr );
	
	resolveOp->interfaceIndex = inInterfaceIndex;
	
	*outOp = resolveOp;
	resolveOp = NULL;
	err = kNoErr;
	
exit:
	if( resolveOp ) BrowseResolveOpFree( resolveOp );
	return( err );
}

//===========================================================================================================================
//	BrowseResolveOpFree
//===========================================================================================================================

static void	BrowseResolveOpFree( BrowseResolveOp *inOp )
{
	DNSServiceForget( &inOp->sdRef );
	ForgetMem( &inOp->fullName );
	free( inOp );
}

//===========================================================================================================================
//	BrowseCallback
//===========================================================================================================================

static void DNSSD_API
	BrowseCallback(
		DNSServiceRef		inSDRef,
		DNSServiceFlags		inFlags,
		uint32_t			inInterfaceIndex,
		DNSServiceErrorType	inError,
		const char *		inName,
		const char *		inRegType,
		const char *		inDomain,
		void *				inContext )
{
	BrowseContext * const		context = (BrowseContext *) inContext;
	OSStatus					err;
	BrowseResolveOp *			newOp = NULL;
	BrowseResolveOp **			p;
	char						fullName[ kDNSServiceMaxDomainName ];
	char						time[ kTimestampBufLen ];
	
	Unused( inSDRef );
	
	GetTimestampStr( time );
	
	err = inError;
	require_noerr( err, exit );
	
	if( !context->printedHeader )
	{
		FPrintF( stdout, "%-26s  A/R Flags IF %-20s %-20s Instance Name\n", "Timestamp", "Domain", "Service Type" );
		context->printedHeader = true;
	}
	FPrintF( stdout, "%-26s  %-3s %5X %2d %-20s %-20s %s\n",
		time, AddRmvString( inFlags ), inFlags, (int32_t) inInterfaceIndex, inDomain, inRegType, inName );
	
	if( !context->doResolve && !context->doResolveTXTOnly ) goto exit;
	
	err = DNSServiceConstructFullName( fullName, inName, inRegType, inDomain );
	require_noerr( err, exit );
	
	if( inFlags & kDNSServiceFlagsAdd )
	{
		DNSServiceRef		sdRef;
		DNSServiceFlags		flags;
		
		err = BrowseResolveOpCreate( fullName, inInterfaceIndex, &newOp );
		require_noerr( err, exit );
		
		if( context->mainRef )
		{
			sdRef = context->mainRef;
			flags = kDNSServiceFlagsShareConnection;
		}
		else
		{
			flags = 0;
		}
		if( context->doResolve )
		{
			err = DNSServiceResolve( &sdRef, flags, inInterfaceIndex, inName, inRegType, inDomain, BrowseResolveCallback,
				NULL );
			require_noerr( err, exit );
		}
		else
		{
			err = DNSServiceQueryRecord( &sdRef, flags, inInterfaceIndex, fullName, kDNSServiceType_TXT, kDNSServiceClass_IN,
				BrowseQueryRecordCallback, NULL );
			require_noerr( err, exit );
		}
		
		newOp->sdRef = sdRef;
		if( !context->mainRef )
		{
			err = DNSServiceSetDispatchQueue( newOp->sdRef, dispatch_get_main_queue() );
			require_noerr( err, exit );
		}
		for( p = &context->resolveList; *p; p = &( *p )->next ) {}
		*p = newOp;
		newOp = NULL;
	}
	else
	{
		BrowseResolveOp *		resolveOp;
		
		for( p = &context->resolveList; ( resolveOp = *p ) != NULL; p = &resolveOp->next )
		{
			if( ( resolveOp->interfaceIndex == inInterfaceIndex ) && ( strcasecmp( resolveOp->fullName, fullName ) == 0 ) )
			{
				break;
			}
		}
		if( resolveOp )
		{
			*p = resolveOp->next;
			BrowseResolveOpFree( resolveOp );
		}
	}
	
exit:
	if( newOp ) BrowseResolveOpFree( newOp );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	BrowseQueryRecordCallback
//===========================================================================================================================

static void DNSSD_API
	BrowseQueryRecordCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		uint16_t				inType,
		uint16_t				inClass,
		uint16_t				inRDataLen,
		const void *			inRDataPtr,
		uint32_t				inTTL,
		void *					inContext )
{
	OSStatus		err;
	char			time[ kTimestampBufLen ];
	
	Unused( inSDRef );
	Unused( inClass );
	Unused( inTTL );
	Unused( inContext );
	
	GetTimestampStr( time );
	
	err = inError;
	require_noerr( err, exit );
	require_action( inType == kDNSServiceType_TXT, exit, err = kTypeErr );
	
	FPrintF( stdout, "%s  %s %s TXT on interface %d\n    TXT: %#{txt}\n",
		time, AddRmvString( inFlags ), inFullName, (int32_t) inInterfaceIndex, inRDataPtr, (size_t) inRDataLen );
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	BrowseResolveCallback
//===========================================================================================================================

static void DNSSD_API
	BrowseResolveCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		const char *			inHostname,
		uint16_t				inPort,
		uint16_t				inTXTLen,
		const unsigned char *	inTXTPtr,
		void *					inContext )
{
	char		time[ kTimestampBufLen ];
	char		errorStr[ 64 ];
	
	Unused( inSDRef );
	Unused( inFlags );
	Unused( inContext );
	
	GetTimestampStr( time );
	
	if( inError ) SNPrintF( errorStr, sizeof( errorStr ), " error %#m", inError );
	
	FPrintF( stdout, "%s  %s can be reached at %s:%u (interface %d)%?s\n",
		time, inFullName, inHostname, ntohs( inPort ), (int32_t) inInterfaceIndex, inError, errorStr );
	if( inTXTLen == 1 )
	{
		FPrintF( stdout, " TXT record: %#H\n", inTXTPtr, (int) inTXTLen, INT_MAX );
	}
	else
	{
		FPrintF( stdout, " TXT record: %#{txt}\n", inTXTPtr, (size_t) inTXTLen );
	}
}

//===========================================================================================================================
//	GetAddrInfoCmd
//===========================================================================================================================

typedef struct
{
	DNSServiceRef			mainRef;		// Main sdRef for shared connection.
	DNSServiceRef			opRef;			// sdRef for the DNSServiceGetAddrInfo operation.
	const char *			name;			// Hostname to resolve.
	DNSServiceFlags			flags;			// Flags argument for DNSServiceGetAddrInfo().
	DNSServiceProtocol		protocols;		// Protocols argument for DNSServiceGetAddrInfo().
	uint32_t				ifIndex;		// Interface index argument for DNSServiceGetAddrInfo().
	int						timeLimitSecs;	// Time limit for the DNSServiceGetAddrInfo() operation in seconds.
	Boolean					printedHeader;	// True if the results header has been printed.
	Boolean					oneShotMode;	// True if command is done after the first set of results (one-shot mode).
	Boolean					needIPv4;		// True if in one-shot mode and an IPv4 result is needed.
	Boolean					needIPv6;		// True if in one-shot mode and an IPv6 result is needed.
	
}	GetAddrInfoContext;

static void	GetAddrInfoPrintPrologue( const GetAddrInfoContext *inContext );
static void	GetAddrInfoContextFree( GetAddrInfoContext *inContext );
static void DNSSD_API
	GetAddrInfoCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inHostname,
		const struct sockaddr *	inSockAddr,
		uint32_t				inTTL,
		void *					inContext );

static void	GetAddrInfoCmd( void )
{
	OSStatus					err;
	DNSServiceRef				sdRef;
	GetAddrInfoContext *		context			= NULL;
	dispatch_source_t			signalSource	= NULL;
	int							useMainConnection;
	
	// Set up SIGINT handler.
	
	signal( SIGINT, SIG_IGN );
	err = DispatchSignalSourceCreate( SIGINT, Exit, kExitReason_SIGINT, &signalSource );
	require_noerr( err, exit );
	dispatch_resume( signalSource );
	
	// Check command parameters.
	
	if( gGetAddrInfo_TimeLimitSecs < 0 )
	{
		FPrintF( stderr, "Invalid time limit: %d s.\n", gGetAddrInfo_TimeLimitSecs );
		err = kParamErr;
		goto exit;
	}
	
	// Create context.
	
	context = (GetAddrInfoContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	// Create main connection.
	
	if( gConnectionOpt )
	{
		err = CreateConnectionFromArgString( gConnectionOpt, dispatch_get_main_queue(), &context->mainRef, NULL );
		require_noerr_quiet( err, exit );
		useMainConnection = true;
	}
	else
	{
		useMainConnection = false;
	}
	
	// Get flags.
	
	context->flags = GetDNSSDFlagsFromOpts();
	if( useMainConnection ) context->flags |= kDNSServiceFlagsShareConnection;
	
	// Get interface.
	
	err = InterfaceIndexFromArgString( gInterface, &context->ifIndex );
	require_noerr_quiet( err, exit );
	
	// Set remaining parameters.
	
	context->name			= gGetAddrInfo_Name;
	context->timeLimitSecs	= gGetAddrInfo_TimeLimitSecs;
	if( gGetAddrInfo_ProtocolIPv4 ) context->protocols |= kDNSServiceProtocol_IPv4;
	if( gGetAddrInfo_ProtocolIPv6 ) context->protocols |= kDNSServiceProtocol_IPv6;
	if( gGetAddrInfo_OneShot )
	{
		context->oneShotMode	= true;
		context->needIPv4		= ( gGetAddrInfo_ProtocolIPv4 || !gGetAddrInfo_ProtocolIPv6 ) ? true : false;
		context->needIPv6		= ( gGetAddrInfo_ProtocolIPv6 || !gGetAddrInfo_ProtocolIPv4 ) ? true : false;
	}
	
	// Print prologue.
	
	GetAddrInfoPrintPrologue( context );
	
	// Start operation.
	
	if( useMainConnection ) sdRef = context->mainRef;
	err = DNSServiceGetAddrInfo( &sdRef, context->flags, context->ifIndex, context->protocols, context->name,
		GetAddrInfoCallback, context );
	require_noerr( err, exit );
	
	context->opRef = sdRef;
	if( !useMainConnection )
	{
		err = DNSServiceSetDispatchQueue( context->opRef, dispatch_get_main_queue() );
		require_noerr( err, exit );
	}
	
	// Set time limit.
	
	if( context->timeLimitSecs > 0 )
	{
		dispatch_after_f( dispatch_time_seconds( context->timeLimitSecs ), dispatch_get_main_queue(),
			kExitReason_TimeLimit, Exit );
	}
	dispatch_main();
	
exit:
	dispatch_source_forget( &signalSource );
	if( context ) GetAddrInfoContextFree( context );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	GetAddrInfoPrintPrologue
//===========================================================================================================================

static void	GetAddrInfoPrintPrologue( const GetAddrInfoContext *inContext )
{
	const int		timeLimitSecs = inContext->timeLimitSecs;
	char			ifName[ kInterfaceNameBufLen ];
	char			time[ kTimestampBufLen ];
	
	InterfaceIndexToName( inContext->ifIndex, ifName );
	
	FPrintF( stdout, "Flags:      %#{flags}\n",	inContext->flags, kDNSServiceFlagsDescriptors );
	FPrintF( stdout, "Interface:  %d (%s)\n",	(int32_t) inContext->ifIndex, ifName );
	FPrintF( stdout, "Protocols:  %#{flags}\n",	inContext->protocols, kDNSServiceProtocolDescriptors );
	FPrintF( stdout, "Name:       %s\n",		inContext->name );
	FPrintF( stdout, "Mode:       %s\n",		inContext->oneShotMode ? "one-shot" : "continuous" );
	FPrintF( stdout, "Time limit: " );
	if( timeLimitSecs > 0 )	FPrintF( stdout, "%d second%?c\n", timeLimitSecs, timeLimitSecs != 1, 's' );
	else					FPrintF( stdout, "∞\n" );
	FPrintF( stdout, "Start time: %s\n", GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
}

//===========================================================================================================================
//	GetAddrInfoContextFree
//===========================================================================================================================

static void	GetAddrInfoContextFree( GetAddrInfoContext *inContext )
{
	DNSServiceForget( &inContext->opRef );
	DNSServiceForget( &inContext->mainRef );
	free( inContext );
}

//===========================================================================================================================
//	GetAddrInfoCallback
//===========================================================================================================================

static void DNSSD_API
	GetAddrInfoCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inHostname,
		const struct sockaddr *	inSockAddr,
		uint32_t				inTTL,
		void *					inContext )
{
	GetAddrInfoContext * const		context = (GetAddrInfoContext *) inContext;
	OSStatus						err;
	const char *					addrStr;
	char							addrStrBuf[ kSockAddrStringMaxSize ];
	char							time[ kTimestampBufLen ];
	
	Unused( inSDRef );
	
	GetTimestampStr( time );
	
	switch( inError )
	{
		case kDNSServiceErr_NoError:
		case kDNSServiceErr_NoSuchRecord:
			err = kNoErr;
			break;
		
		case kDNSServiceErr_Timeout:
			Exit( kExitReason_Timeout );
		
		default:
			err = inError;
			goto exit;
	}
	
	if( ( inSockAddr->sa_family != AF_INET ) && ( inSockAddr->sa_family != AF_INET6 ) )
	{
		dlogassert( "Unexpected address family: %d", inSockAddr->sa_family );
		err = kTypeErr;
		goto exit;
	}
	
	if( !inError )
	{
		err = SockAddrToString( inSockAddr, kSockAddrStringFlagsNone, addrStrBuf );
		require_noerr( err, exit );
		addrStr = addrStrBuf;
	}
	else
	{
		addrStr = ( inSockAddr->sa_family == AF_INET ) ? "No Such Record (A)" : "No Such Record (AAAA)";
	}
	
	if( !context->printedHeader )
	{
		FPrintF( stdout, "%-26s  A/R Flags IF %-32s %-38s %6s\n", "Timestamp", "Hostname", "Address", "TTL" );
		context->printedHeader = true;
	}
	FPrintF( stdout, "%-26s  %s %5X %2d %-32s %-38s %6u\n",
		time, AddRmvString( inFlags ), inFlags, (int32_t) inInterfaceIndex, inHostname, addrStr, inTTL );
	
	if( context->oneShotMode )
	{
		if( inFlags & kDNSServiceFlagsAdd )
		{
			if( inSockAddr->sa_family == AF_INET )	context->needIPv4 = false;
			else									context->needIPv6 = false;
		}
		if( !( inFlags & kDNSServiceFlagsMoreComing ) && !context->needIPv4 && !context->needIPv6 )
		{
			Exit( kExitReason_OneShotDone );
		}
	}
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	QueryRecordCmd
//===========================================================================================================================

typedef struct
{
	DNSServiceRef		mainRef;		// Main sdRef for shared connection.
	DNSServiceRef		opRef;			// sdRef for the DNSServiceQueryRecord operation.
	const char *		recordName;		// Resource record name argument for DNSServiceQueryRecord().
	DNSServiceFlags		flags;			// Flags argument for DNSServiceQueryRecord().
	uint32_t			ifIndex;		// Interface index argument for DNSServiceQueryRecord().
	int					timeLimitSecs;	// Time limit for the DNSServiceQueryRecord() operation in seconds.
	uint16_t			recordType;		// Resource record type argument for DNSServiceQueryRecord().
	Boolean				printedHeader;	// True if the results header was printed.
	Boolean				oneShotMode;	// True if command is done after the first set of results (one-shot mode).
	Boolean				gotRecord;		// True if in one-shot mode and received at least one record of the desired type.
	Boolean				printRawRData;	// True if RDATA results are not to be formatted.
	
}	QueryRecordContext;

static void	QueryRecordPrintPrologue( const QueryRecordContext *inContext );
static void	QueryRecordContextFree( QueryRecordContext *inContext );
static void DNSSD_API
	QueryRecordCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		uint16_t				inType,
		uint16_t				inClass,
		uint16_t				inRDataLen,
		const void *			inRDataPtr,
		uint32_t				inTTL,
		void *					inContext );

static void	QueryRecordCmd( void )
{
	OSStatus					err;
	DNSServiceRef				sdRef;
	QueryRecordContext *		context			= NULL;
	dispatch_source_t			signalSource	= NULL;
	int							useMainConnection;
	
	// Set up SIGINT handler.
	
	signal( SIGINT, SIG_IGN );
	err = DispatchSignalSourceCreate( SIGINT, Exit, kExitReason_SIGINT, &signalSource );
	require_noerr( err, exit );
	dispatch_resume( signalSource );
	
	// Create context.
	
	context = (QueryRecordContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	// Check command parameters.
	
	if( gQueryRecord_TimeLimitSecs < 0 )
	{
		FPrintF( stderr, "Invalid time limit: %d seconds.\n", gQueryRecord_TimeLimitSecs );
		err = kParamErr;
		goto exit;
	}
	
	// Create main connection.
	
	if( gConnectionOpt )
	{
		err = CreateConnectionFromArgString( gConnectionOpt, dispatch_get_main_queue(), &context->mainRef, NULL );
		require_noerr_quiet( err, exit );
		useMainConnection = true;
	}
	else
	{
		useMainConnection = false;
	}
	
	// Get flags.
	
	context->flags = GetDNSSDFlagsFromOpts();
	if( useMainConnection ) context->flags |= kDNSServiceFlagsShareConnection;
	
	// Get interface.
	
	err = InterfaceIndexFromArgString( gInterface, &context->ifIndex );
	require_noerr_quiet( err, exit );
	
	// Get record type.
	
	err = RecordTypeFromArgString( gQueryRecord_Type, &context->recordType );
	require_noerr( err, exit );
	
	// Set remaining parameters.
	
	context->recordName		= gQueryRecord_Name;
	context->timeLimitSecs	= gQueryRecord_TimeLimitSecs;
	context->oneShotMode	= gQueryRecord_OneShot	? true : false;
	context->printRawRData	= gQueryRecord_RawRData	? true : false;
	
	// Print prologue.
	
	QueryRecordPrintPrologue( context );
	
	// Start operation.
	
	if( useMainConnection ) sdRef = context->mainRef;
	err = DNSServiceQueryRecord( &sdRef, context->flags, context->ifIndex, context->recordName, context->recordType,
		kDNSServiceClass_IN, QueryRecordCallback, context );
	require_noerr( err, exit );
	
	context->opRef = sdRef;
	if( !useMainConnection )
	{
		err = DNSServiceSetDispatchQueue( context->opRef, dispatch_get_main_queue() );
		require_noerr( err, exit );
	}
	
	// Set time limit.
	
	if( context->timeLimitSecs > 0 )
	{
		dispatch_after_f( dispatch_time_seconds( context->timeLimitSecs ), dispatch_get_main_queue(), kExitReason_TimeLimit,
			Exit );
	}
	dispatch_main();
	
exit:
	dispatch_source_forget( &signalSource );
	if( context ) QueryRecordContextFree( context );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	QueryRecordContextFree
//===========================================================================================================================

static void	QueryRecordContextFree( QueryRecordContext *inContext )
{
	DNSServiceForget( &inContext->opRef );
	DNSServiceForget( &inContext->mainRef );
	free( inContext );
}

//===========================================================================================================================
//	QueryRecordPrintPrologue
//===========================================================================================================================

static void	QueryRecordPrintPrologue( const QueryRecordContext *inContext )
{
	const int		timeLimitSecs = inContext->timeLimitSecs;
	char			ifName[ kInterfaceNameBufLen ];
	char			time[ kTimestampBufLen ];
	
	InterfaceIndexToName( inContext->ifIndex, ifName );
	
	FPrintF( stdout, "Flags:       %#{flags}\n",	inContext->flags, kDNSServiceFlagsDescriptors );
	FPrintF( stdout, "Interface:   %d (%s)\n",		(int32_t) inContext->ifIndex, ifName );
	FPrintF( stdout, "Name:        %s\n",			inContext->recordName );
	FPrintF( stdout, "Type:        %s (%u)\n",		RecordTypeToString( inContext->recordType ), inContext->recordType );
	FPrintF( stdout, "Mode:        %s\n",			inContext->oneShotMode ? "one-shot" : "continuous" );
	FPrintF( stdout, "Time limit:  " );
	if( timeLimitSecs > 0 )	FPrintF( stdout, "%d second%?c\n", timeLimitSecs, timeLimitSecs != 1, 's' );
	else					FPrintF( stdout, "∞\n" );
	FPrintF( stdout, "Start time:  %s\n", GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
	
}

//===========================================================================================================================
//	QueryRecordCallback
//===========================================================================================================================

static void DNSSD_API
	QueryRecordCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		uint16_t				inType,
		uint16_t				inClass,
		uint16_t				inRDataLen,
		const void *			inRDataPtr,
		uint32_t				inTTL,
		void *					inContext )
{
	QueryRecordContext * const		context		= (QueryRecordContext *) inContext;
	OSStatus						err;
	char *							rdataStr	= NULL;
	char							time[ kTimestampBufLen ];
	
	Unused( inSDRef );
	
	GetTimestampStr( time );
	
	switch( inError )
	{
		case kDNSServiceErr_NoError:
		case kDNSServiceErr_NoSuchRecord:
			err = kNoErr;
			break;
		
		case kDNSServiceErr_Timeout:
			Exit( kExitReason_Timeout );
		
		default:
			err = inError;
			goto exit;
	}
	
	if( inError == kDNSServiceErr_NoSuchRecord )
	{
		ASPrintF( &rdataStr, "No Such Record" );
	}
	else
	{
		if( !context->printRawRData ) DNSRecordDataToString( inRDataPtr, inRDataLen, inType, NULL, 0, &rdataStr );
		if( !rdataStr )
		{
			ASPrintF( &rdataStr, "%#H", inRDataPtr, inRDataLen, INT_MAX );
			require_action( rdataStr, exit, err = kNoMemoryErr );
		}
	}
	
	if( !context->printedHeader )
	{
		FPrintF( stdout, "%-26s  A/R Flags IF %-32s %-5s %-5s %6s RData\n", "Timestamp", "Name", "Type", "Class", "TTL" );
		context->printedHeader = true;
	}
	FPrintF( stdout, "%-26s  %-3s %5X %2d %-32s %-5s %?-5s%?5u %6u %s\n",
		time, AddRmvString( inFlags ), inFlags, (int32_t) inInterfaceIndex, inFullName, RecordTypeToString( inType ),
		( inClass == kDNSServiceClass_IN ), "IN", ( inClass != kDNSServiceClass_IN ), inClass, inTTL, rdataStr );
	
	if( context->oneShotMode )
	{
		if( ( inFlags & kDNSServiceFlagsAdd ) &&
			( ( context->recordType == kDNSServiceType_ANY ) || ( context->recordType == inType ) ) )
		{
			context->gotRecord = true;
		}
		if( !( inFlags & kDNSServiceFlagsMoreComing ) && context->gotRecord ) Exit( kExitReason_OneShotDone );
	}
	
exit:
	FreeNullSafe( rdataStr );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	RegisterCmd
//===========================================================================================================================

typedef struct
{
	DNSRecordRef		recordRef;	// Reference returned by DNSServiceAddRecord().
	uint8_t *			dataPtr;	// Record data.
	size_t				dataLen;	// Record data length.
	uint32_t			ttl;		// Record TTL value.
	uint16_t			type;		// Record type.
	
}	ExtraRecord;

typedef struct
{
	DNSServiceRef		opRef;				// sdRef for DNSServiceRegister operation.
	const char *		name;				// Service name argument for DNSServiceRegister().
	const char *		type;				// Service type argument for DNSServiceRegister().
	const char *		domain;				// Domain in which advertise the service.
	uint8_t *			txtPtr;				// Service TXT record data. (malloc'd)
	size_t				txtLen;				// Service TXT record data len.
	ExtraRecord *		extraRecords;		// Array of extra records to add to registered service.
	size_t				extraRecordsCount;	// Number of extra records.
	uint8_t *			updateTXTPtr;		// Pointer to record data for TXT record update. (malloc'd)
	size_t				updateTXTLen;		// Length of record data for TXT record update.
	uint32_t			updateTTL;			// TTL of updated TXT record.
	int					updateDelayMs;		// Post-registration TXT record update delay in milliseconds.
	DNSServiceFlags		flags;				// Flags argument for DNSServiceRegister().
	uint32_t			ifIndex;			// Interface index argument for DNSServiceRegister().
	int					lifetimeMs;			// Lifetime of the record registration in milliseconds.
	uint16_t			port;				// Service instance's port number.
	Boolean				printedHeader;		// True if results header was printed.
	Boolean				didRegister;		// True if service was registered.
	
}	RegisterContext;

static void	RegisterPrintPrologue( const RegisterContext *inContext );
static void	RegisterContextFree( RegisterContext *inContext );
static void DNSSD_API
	RegisterCallback(
		DNSServiceRef		inSDRef,
		DNSServiceFlags		inFlags,
		DNSServiceErrorType	inError,
		const char *		inName,
		const char *		inType,
		const char *		inDomain,
		void *				inContext );
static void	RegisterUpdate( void *inContext );

static void	RegisterCmd( void )
{
	OSStatus				err;
	RegisterContext *		context			= NULL;
	dispatch_source_t		signalSource	= NULL;
	
	// Set up SIGINT handler.
	
	signal( SIGINT, SIG_IGN );
	err = DispatchSignalSourceCreate( SIGINT, Exit, kExitReason_SIGINT, &signalSource );
	require_noerr( err, exit );
	dispatch_resume( signalSource );
	
	// Create context.
	
	context = (RegisterContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	// Check command parameters.
	
	if( ( gRegister_Port < 0 ) || ( gRegister_Port > UINT16_MAX ) )
	{
		FPrintF( stderr, "Port number %d is out-of-range.\n", gRegister_Port );
		err = kParamErr;
		goto exit;
	}
	
	if( ( gAddRecord_DataCount != gAddRecord_TypesCount ) || ( gAddRecord_TTLsCount != gAddRecord_TypesCount ) )
	{
		FPrintF( stderr, "There are missing additional record parameters.\n" );
		err = kParamErr;
		goto exit;
	}
	
	// Get flags.
	
	context->flags = GetDNSSDFlagsFromOpts();
	
	// Get interface index.
	
	err = InterfaceIndexFromArgString( gInterface, &context->ifIndex );
	require_noerr_quiet( err, exit );
	
	// Get TXT record data.
	
	if( gRegister_TXT )
	{
		err = RecordDataFromArgString( gRegister_TXT, &context->txtPtr, &context->txtLen );
		require_noerr_quiet( err, exit );
	}
	
	// Set remaining parameters.
	
	context->name		= gRegister_Name;
	context->type		= gRegister_Type;
	context->domain		= gRegister_Domain;
	context->port		= (uint16_t) gRegister_Port;
	context->lifetimeMs	= gRegister_LifetimeMs;
	
	if( gAddRecord_TypesCount > 0 )
	{
		size_t		i;
		
		context->extraRecords = (ExtraRecord *) calloc( gAddRecord_TypesCount, sizeof( ExtraRecord ) );
		require_action( context, exit, err = kNoMemoryErr );
		context->extraRecordsCount = gAddRecord_TypesCount;
		
		for( i = 0; i < gAddRecord_TypesCount; ++i )
		{
			ExtraRecord * const		extraRecord = &context->extraRecords[ i ];
			
			err = RecordTypeFromArgString( gAddRecord_Types[ i ], &extraRecord->type );
			require_noerr( err, exit );
			
			err = StringToUInt32( gAddRecord_TTLs[ i ], &extraRecord->ttl );
			if( err )
			{
				FPrintF( stderr, "Invalid TTL value: %s\n", gAddRecord_TTLs[ i ] );
				err = kParamErr;
				goto exit;
			}
			
			err = RecordDataFromArgString( gAddRecord_Data[ i ], &extraRecord->dataPtr, &extraRecord->dataLen );
			require_noerr_quiet( err, exit );
		}
	}
	
	if( gUpdateRecord_Data )
	{
		err = RecordDataFromArgString( gUpdateRecord_Data, &context->updateTXTPtr, &context->updateTXTLen );
		require_noerr_quiet( err, exit );
		
		context->updateTTL		= (uint32_t) gUpdateRecord_TTL;
		context->updateDelayMs	= gUpdateRecord_DelayMs;
	}
	
	// Print prologue.
	
	RegisterPrintPrologue( context );
	
	// Start operation.
	
	err = DNSServiceRegister( &context->opRef, context->flags, context->ifIndex, context->name, context->type,
		context->domain, NULL, ntohs( context->port ), (uint16_t) context->txtLen, context->txtPtr,
		RegisterCallback, context );
	ForgetMem( &context->txtPtr );
	require_noerr( err, exit );
	
	err = DNSServiceSetDispatchQueue( context->opRef, dispatch_get_main_queue() );
	require_noerr( err, exit );
	
	dispatch_main();
	
exit:
	dispatch_source_forget( &signalSource );
	if( context ) RegisterContextFree( context );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	RegisterPrintPrologue
//===========================================================================================================================

static void	RegisterPrintPrologue( const RegisterContext *inContext )
{
	size_t		i;
	int			infinite;
	char		ifName[ kInterfaceNameBufLen ];
	char		time[ kTimestampBufLen ];
	
	InterfaceIndexToName( inContext->ifIndex, ifName );
	
	FPrintF( stdout, "Flags:      %#{flags}\n",	inContext->flags, kDNSServiceFlagsDescriptors );
	FPrintF( stdout, "Interface:  %d (%s)\n",	(int32_t) inContext->ifIndex, ifName );
	FPrintF( stdout, "Name:       %s\n",		inContext->name ? inContext->name : "<NULL>" );
	FPrintF( stdout, "Type:       %s\n",		inContext->type );
	FPrintF( stdout, "Domain:     %s\n",		inContext->domain ? inContext->domain : "<NULL> (default domains)" );
	FPrintF( stdout, "Port:       %u\n",		inContext->port );
	FPrintF( stdout, "TXT data:   %#{txt}\n",	inContext->txtPtr, inContext->txtLen );
	infinite = ( inContext->lifetimeMs < 0 ) ? true : false;
	FPrintF( stdout, "Lifetime:   %?s%?d ms\n",	infinite, "∞", !infinite, inContext->lifetimeMs );
	if( inContext->updateTXTPtr )
	{
		FPrintF( stdout, "\nUpdate record:\n" );
		FPrintF( stdout, "    Delay:    %d ms\n",	( inContext->updateDelayMs > 0 ) ? inContext->updateDelayMs : 0 );
		FPrintF( stdout, "    TTL:      %u%?s\n",
			inContext->updateTTL, inContext->updateTTL == 0, " (system will use a default value.)" );
		FPrintF( stdout, "    TXT data: %#{txt}\n",	inContext->updateTXTPtr, inContext->updateTXTLen );
	}
	if( inContext->extraRecordsCount > 0 ) FPrintF( stdout, "\n" );
	for( i = 0; i < inContext->extraRecordsCount; ++i )
	{
		const ExtraRecord *		record = &inContext->extraRecords[ i ];
		
		FPrintF( stdout, "Extra record %zu:\n",		i + 1 );
		FPrintF( stdout, "    Type:  %s (%u)\n",	RecordTypeToString( record->type ), record->type );
		FPrintF( stdout, "    TTL:   %u%?s\n",		record->ttl, record->ttl == 0, " (system will use a default value.)" );
		FPrintF( stdout, "    RData: %#H\n\n",		record->dataPtr, (int) record->dataLen, INT_MAX );
	}
	FPrintF( stdout, "Start time: %s\n", GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
}

//===========================================================================================================================
//	RegisterContextFree
//===========================================================================================================================

static void	RegisterContextFree( RegisterContext *inContext )
{
	ExtraRecord *					record;
	const ExtraRecord * const		end = inContext->extraRecords + inContext->extraRecordsCount;
	
	DNSServiceForget( &inContext->opRef );
	ForgetMem( &inContext->txtPtr );
	for( record = inContext->extraRecords; record < end; ++record )
	{
		check( !record->recordRef );
		ForgetMem( &record->dataPtr );
	}
	ForgetMem( &inContext->extraRecords );
	ForgetMem( &inContext->updateTXTPtr );
	free( inContext );
}

//===========================================================================================================================
//	RegisterCallback
//===========================================================================================================================

static void DNSSD_API
	RegisterCallback(
		DNSServiceRef		inSDRef,
		DNSServiceFlags		inFlags,
		DNSServiceErrorType	inError,
		const char *		inName,
		const char *		inType,
		const char *		inDomain,
		void *				inContext )
{
	RegisterContext * const		context = (RegisterContext *) inContext;
	OSStatus					err;
	char						time[ kTimestampBufLen ];
	
	Unused( inSDRef );
	
	GetTimestampStr( time );
	
	if( !context->printedHeader )
	{
		FPrintF( stdout, "%-26s  A/R Flags Service\n", "Timestamp" );
		context->printedHeader = true;
	}
	FPrintF( stdout, "%-26s  %-3s %5X %s.%s%s %?#m\n",
		time, AddRmvString( inFlags ), inFlags, inName, inType, inDomain, inError, inError );
	
	require_noerr_action_quiet( inError, exit, err = inError );
	
	if( !context->didRegister && ( inFlags & kDNSServiceFlagsAdd ) )
	{
		context->didRegister = true;
		if( context->updateTXTPtr )
		{
			if( context->updateDelayMs > 0 )
			{
				dispatch_after_f( dispatch_time_milliseconds( context->updateDelayMs ), dispatch_get_main_queue(),
					context, RegisterUpdate );
			}
			else
			{
				RegisterUpdate( context );
			}
		}
		if( context->extraRecordsCount > 0 )
		{
			ExtraRecord *					record;
			const ExtraRecord * const		end = context->extraRecords + context->extraRecordsCount;
			
			for( record = context->extraRecords; record < end; ++record )
			{
				err = DNSServiceAddRecord( context->opRef, &record->recordRef, 0, record->type,
					(uint16_t) record->dataLen, record->dataPtr, record->ttl );
				require_noerr( err, exit );
			}
		}
		if( context->lifetimeMs == 0 )
		{
			Exit( kExitReason_TimeLimit );
		}
		else if( context->lifetimeMs > 0 )
		{
			dispatch_after_f( dispatch_time_milliseconds( context->lifetimeMs ), dispatch_get_main_queue(),
				kExitReason_TimeLimit, Exit );
		}
	}
	err = kNoErr;
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	RegisterUpdate
//===========================================================================================================================

static void	RegisterUpdate( void *inContext )
{
	OSStatus					err;
	RegisterContext * const		context = (RegisterContext *) inContext;
	
	err = DNSServiceUpdateRecord( context->opRef, NULL, 0, (uint16_t) context->updateTXTLen, context->updateTXTPtr,
		context->updateTTL );
	require_noerr( err, exit );
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	RegisterRecordCmd
//===========================================================================================================================

typedef struct
{
	DNSServiceRef		conRef;			// sdRef to be initialized by DNSServiceCreateConnection().
	DNSRecordRef		recordRef;		// Registered record reference.
	const char *		recordName;		// Name of resource record.
	uint8_t *			dataPtr;		// Pointer to resource record data.
	size_t				dataLen;		// Length of resource record data.
	uint32_t			ttl;			// TTL value of resource record in seconds.
	uint32_t			ifIndex;		// Interface index argument for DNSServiceRegisterRecord().
	DNSServiceFlags		flags;			// Flags argument for DNSServiceRegisterRecord().
	int					lifetimeMs;		// Lifetime of the record registration in milliseconds.
	uint16_t			recordType;		// Resource record type.
	uint8_t *			updateDataPtr;	// Pointer to data for record update. (malloc'd)
	size_t				updateDataLen;	// Length of data for record update.
	uint32_t			updateTTL;		// TTL for updated record.
	int					updateDelayMs;	// Post-registration record update delay in milliseconds.
	Boolean				didRegister;	// True if the record was registered.
	
}	RegisterRecordContext;

static void	RegisterRecordPrintPrologue( const RegisterRecordContext *inContext );
static void	RegisterRecordContextFree( RegisterRecordContext *inContext );
static void DNSSD_API
	RegisterRecordCallback(
		DNSServiceRef		inSDRef,
		DNSRecordRef		inRecordRef,
		DNSServiceFlags		inFlags,
		DNSServiceErrorType	inError,
		void *				inContext );
static void	RegisterRecordUpdate( void *inContext );

static void	RegisterRecordCmd( void )
{
	OSStatus					err;
	RegisterRecordContext *		context			= NULL;
	dispatch_source_t			signalSource	= NULL;
	
	// Set up SIGINT handler.
	
	signal( SIGINT, SIG_IGN );
	err = DispatchSignalSourceCreate( SIGINT, Exit, kExitReason_SIGINT, &signalSource );
	require_noerr( err, exit );
	dispatch_resume( signalSource );
	
	// Create context.
	
	context = (RegisterRecordContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	// Create connection.
	
	err = CreateConnectionFromArgString( gConnectionOpt, dispatch_get_main_queue(), &context->conRef, NULL );
	require_noerr_quiet( err, exit );
	
	// Get flags.
	
	context->flags = GetDNSSDFlagsFromOpts();
	
	// Get interface.
	
	err = InterfaceIndexFromArgString( gInterface, &context->ifIndex );
	require_noerr_quiet( err, exit );
	
	// Get record type.
	
	err = RecordTypeFromArgString( gRegisterRecord_Type, &context->recordType );
	require_noerr( err, exit );
	
	// Get record data.
	
	if( gRegisterRecord_Data )
	{
		err = RecordDataFromArgString( gRegisterRecord_Data, &context->dataPtr, &context->dataLen );
		require_noerr_quiet( err, exit );
	}
	
	// Set remaining parameters.
	
	context->recordName	= gRegisterRecord_Name;
	context->ttl		= (uint32_t) gRegisterRecord_TTL;
	context->lifetimeMs	= gRegisterRecord_LifetimeMs;
	
	// Get update data.
	
	if( gRegisterRecord_UpdateData )
	{
		err = RecordDataFromArgString( gRegisterRecord_UpdateData, &context->updateDataPtr, &context->updateDataLen );
		require_noerr_quiet( err, exit );
		
		context->updateTTL		= (uint32_t) gRegisterRecord_UpdateTTL;
		context->updateDelayMs	= gRegisterRecord_UpdateDelayMs;
	}
	
	// Print prologue.
	
	RegisterRecordPrintPrologue( context );
	
	// Start operation.
	
	err = DNSServiceRegisterRecord( context->conRef, &context->recordRef, context->flags, context->ifIndex,
		context->recordName, context->recordType, kDNSServiceClass_IN, (uint16_t) context->dataLen, context->dataPtr,
		context->ttl, RegisterRecordCallback, context );
	if( err )
	{
		FPrintF( stderr, "DNSServiceRegisterRecord() returned %#m\n", err );
		goto exit;
	}
	
	dispatch_main();
	
exit:
	dispatch_source_forget( &signalSource );
	if( context ) RegisterRecordContextFree( context );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	RegisterRecordPrintPrologue
//===========================================================================================================================

static void	RegisterRecordPrintPrologue( const RegisterRecordContext *inContext )
{
	int			infinite;
	char		time[ kTimestampBufLen ];
	char		ifName[ kInterfaceNameBufLen ];
	
	InterfaceIndexToName( inContext->ifIndex, ifName );
	
	FPrintF( stdout, "Flags:       %#{flags}\n",	inContext->flags, kDNSServiceFlagsDescriptors );
	FPrintF( stdout, "Interface:   %d (%s)\n",		(int32_t) inContext->ifIndex, ifName );
	FPrintF( stdout, "Name:        %s\n",			inContext->recordName );
	FPrintF( stdout, "Type:        %s (%u)\n",		RecordTypeToString( inContext->recordType ), inContext->recordType );
	FPrintF( stdout, "TTL:         %u\n",			inContext->ttl );
	FPrintF( stdout, "Data:        %#H\n",			inContext->dataPtr, (int) inContext->dataLen, INT_MAX );
	infinite = ( inContext->lifetimeMs < 0 ) ? true : false;
	FPrintF( stdout, "Lifetime:    %?s%?d ms\n",	infinite, "∞", !infinite, inContext->lifetimeMs );
	if( inContext->updateDataPtr )
	{
		FPrintF( stdout, "\nUpdate record:\n" );
		FPrintF( stdout, "    Delay:    %d ms\n",	( inContext->updateDelayMs >= 0 ) ? inContext->updateDelayMs : 0 );
		FPrintF( stdout, "    TTL:      %u%?s\n",
			inContext->updateTTL, inContext->updateTTL == 0, " (system will use a default value.)" );
		FPrintF( stdout, "    RData:    %#H\n",		inContext->updateDataPtr, (int) inContext->updateDataLen, INT_MAX );
	}
	FPrintF( stdout, "Start time:  %s\n",			GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
}

//===========================================================================================================================
//	RegisterRecordContextFree
//===========================================================================================================================

static void	RegisterRecordContextFree( RegisterRecordContext *inContext )
{
	DNSServiceForget( &inContext->conRef );
	ForgetMem( &inContext->dataPtr );
	ForgetMem( &inContext->updateDataPtr );
	free( inContext );
}

//===========================================================================================================================
//	RegisterRecordCallback
//===========================================================================================================================

static void
	RegisterRecordCallback(
		DNSServiceRef		inSDRef,
		DNSRecordRef		inRecordRef,
		DNSServiceFlags		inFlags,
		DNSServiceErrorType	inError,
		void *				inContext )
{
	RegisterRecordContext *		context = (RegisterRecordContext *) inContext;
	char						time[ kTimestampBufLen ];
	
	Unused( inSDRef );
	Unused( inRecordRef );
	Unused( inFlags );
	Unused( context );
	
	GetTimestampStr( time );
	FPrintF( stdout, "%s Record registration result (error %#m)\n", time, inError );
	
	if( !context->didRegister && !inError )
	{
		context->didRegister = true;
		if( context->updateDataPtr )
		{
			if( context->updateDelayMs > 0 )
			{
				dispatch_after_f( dispatch_time_milliseconds( context->updateDelayMs ), dispatch_get_main_queue(),
					context, RegisterRecordUpdate );
			}
			else
			{
				RegisterRecordUpdate( context );
			}
		}
		if( context->lifetimeMs == 0 )
		{
			Exit( kExitReason_TimeLimit );
		}
		else if( context->lifetimeMs > 0 )
		{
			dispatch_after_f( dispatch_time_milliseconds( context->lifetimeMs ), dispatch_get_main_queue(),
				kExitReason_TimeLimit, Exit );
		}
	}
}

//===========================================================================================================================
//	RegisterRecordUpdate
//===========================================================================================================================

static void	RegisterRecordUpdate( void *inContext )
{
	OSStatus							err;
	RegisterRecordContext * const		context = (RegisterRecordContext *) inContext;
	
	err = DNSServiceUpdateRecord( context->conRef, context->recordRef, 0, (uint16_t) context->updateDataLen,
		context->updateDataPtr, context->updateTTL );
	require_noerr( err, exit );
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	ResolveCmd
//===========================================================================================================================

typedef struct
{
	DNSServiceRef		mainRef;		// Main sdRef for shared connections.
	DNSServiceRef		opRef;			// sdRef for the DNSServiceResolve operation.
	DNSServiceFlags		flags;			// Flags argument for DNSServiceResolve().
	const char *		name;			// Service name argument for DNSServiceResolve().
	const char *		type;			// Service type argument for DNSServiceResolve().
	const char *		domain;			// Domain argument for DNSServiceResolve().
	uint32_t			ifIndex;		// Interface index argument for DNSServiceResolve().
	int					timeLimitSecs;	// Time limit for the DNSServiceResolve operation in seconds.
	
}	ResolveContext;

static void	ResolvePrintPrologue( const ResolveContext *inContext );
static void	ResolveContextFree( ResolveContext *inContext );
static void DNSSD_API
	ResolveCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		const char *			inHostname,
		uint16_t				inPort,
		uint16_t				inTXTLen,
		const unsigned char *	inTXTPtr,
		void *					inContext );

static void	ResolveCmd( void )
{
	OSStatus				err;
	DNSServiceRef			sdRef;
	ResolveContext *		context			= NULL;
	dispatch_source_t		signalSource	= NULL;
	int						useMainConnection;
	
	// Set up SIGINT handler.
	
	signal( SIGINT, SIG_IGN );
	err = DispatchSignalSourceCreate( SIGINT, Exit, kExitReason_SIGINT, &signalSource );
	require_noerr( err, exit );
	dispatch_resume( signalSource );
	
	// Create context.
	
	context = (ResolveContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	// Check command parameters.
	
	if( gResolve_TimeLimitSecs < 0 )
	{
		FPrintF( stderr, "Invalid time limit: %d seconds.\n", gResolve_TimeLimitSecs );
		err = kParamErr;
		goto exit;
	}
	
	// Create main connection.
	
	if( gConnectionOpt )
	{
		err = CreateConnectionFromArgString( gConnectionOpt, dispatch_get_main_queue(), &context->mainRef, NULL );
		require_noerr_quiet( err, exit );
		useMainConnection = true;
	}
	else
	{
		useMainConnection = false;
	}
	
	// Get flags.
	
	context->flags = GetDNSSDFlagsFromOpts();
	if( useMainConnection ) context->flags |= kDNSServiceFlagsShareConnection;
	
	// Get interface index.
	
	err = InterfaceIndexFromArgString( gInterface, &context->ifIndex );
	require_noerr_quiet( err, exit );
	
	// Set remaining parameters.
	
	context->name			= gResolve_Name;
	context->type			= gResolve_Type;
	context->domain			= gResolve_Domain;
	context->timeLimitSecs	= gResolve_TimeLimitSecs;
	
	// Print prologue.
	
	ResolvePrintPrologue( context );
	
	// Start operation.
	
	if( useMainConnection ) sdRef = context->mainRef;
	err = DNSServiceResolve( &sdRef, context->flags, context->ifIndex, context->name, context->type, context->domain,
		ResolveCallback, NULL );
	require_noerr( err, exit );
	
	context->opRef = sdRef;
	if( !useMainConnection )
	{
		err = DNSServiceSetDispatchQueue( context->opRef, dispatch_get_main_queue() );
		require_noerr( err, exit );
	}
	
	// Set time limit.
	
	if( context->timeLimitSecs > 0 )
	{
		dispatch_after_f( dispatch_time_seconds( context->timeLimitSecs ), dispatch_get_main_queue(),
			kExitReason_TimeLimit, Exit );
	}
	dispatch_main();
	
exit:
	dispatch_source_forget( &signalSource );
	if( context ) ResolveContextFree( context );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	ReconfirmCmd
//===========================================================================================================================

static void	ReconfirmCmd( void )
{
	OSStatus			err;
	uint8_t *			rdataPtr = NULL;
	size_t				rdataLen = 0;
	DNSServiceFlags		flags;
	uint32_t			ifIndex;
	uint16_t			type, class;
	char				ifName[ kInterfaceNameBufLen ];
	
	// Get flags.
	
	flags = GetDNSSDFlagsFromOpts();
	
	// Get interface index.
	
	err = InterfaceIndexFromArgString( gInterface, &ifIndex );
	require_noerr_quiet( err, exit );
	
	// Get record type.
	
	err = RecordTypeFromArgString( gReconfirmRecord_Type, &type );
	require_noerr( err, exit );
	
	// Get record data.
	
	if( gReconfirmRecord_Data )
	{
		err = RecordDataFromArgString( gReconfirmRecord_Data, &rdataPtr, &rdataLen );
		require_noerr_quiet( err, exit );
	}
	
	// Get record data.
	
	if( gReconfirmRecord_Class )
	{
		err = RecordClassFromArgString( gReconfirmRecord_Class, &class );
		require_noerr( err, exit );
	}
	else
	{
		class = kDNSServiceClass_IN;
	}
	
	// Print prologue.
	
	FPrintF( stdout, "Flags:     %#{flags}\n",	flags, kDNSServiceFlagsDescriptors );
	FPrintF( stdout, "Interface: %d (%s)\n",	(int32_t) ifIndex, InterfaceIndexToName( ifIndex, ifName ) );
	FPrintF( stdout, "Name:      %s\n",			gReconfirmRecord_Name );
	FPrintF( stdout, "Type:      %s (%u)\n",	RecordTypeToString( type ), type );
	FPrintF( stdout, "Class:     %s (%u)\n",	( class == kDNSServiceClass_IN ) ? "IN" : "???", class );
	FPrintF( stdout, "Data:      %#H\n",		rdataPtr, (int) rdataLen, INT_MAX );
	FPrintF( stdout, "---\n" );
	
	err = DNSServiceReconfirmRecord( flags, ifIndex, gReconfirmRecord_Name, type, class, (uint16_t) rdataLen, rdataPtr );
	FPrintF( stdout, "Error:     %#m\n", err );
	
exit:
	FreeNullSafe( rdataPtr );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	ResolvePrintPrologue
//===========================================================================================================================

static void	ResolvePrintPrologue( const ResolveContext *inContext )
{
	const int		timeLimitSecs = inContext->timeLimitSecs;
	char			ifName[ kInterfaceNameBufLen ];
	char			time[ kTimestampBufLen ];
	
	InterfaceIndexToName( inContext->ifIndex, ifName );
	
	FPrintF( stdout, "Flags:      %#{flags}\n",	inContext->flags, kDNSServiceFlagsDescriptors );
	FPrintF( stdout, "Interface:  %d (%s)\n",	(int32_t) inContext->ifIndex, ifName );
	FPrintF( stdout, "Name:       %s\n",		inContext->name );
	FPrintF( stdout, "Type:       %s\n",		inContext->type );
	FPrintF( stdout, "Domain:     %s\n",		inContext->domain );
	FPrintF( stdout, "Time limit: " );
	if( timeLimitSecs > 0 )	FPrintF( stdout, "%d second%?c\n", timeLimitSecs, timeLimitSecs != 1, 's' );
	else					FPrintF( stdout, "∞\n" );
	FPrintF( stdout, "Start time: %s\n", GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
}

//===========================================================================================================================
//	ResolveContextFree
//===========================================================================================================================

static void	ResolveContextFree( ResolveContext *inContext )
{
	DNSServiceForget( &inContext->opRef );
	DNSServiceForget( &inContext->mainRef );
	free( inContext );
}

//===========================================================================================================================
//	ResolveCallback
//===========================================================================================================================

static void DNSSD_API
	ResolveCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		const char *			inHostname,
		uint16_t				inPort,
		uint16_t				inTXTLen,
		const unsigned char *	inTXTPtr,
		void *					inContext )
{
	char		time[ kTimestampBufLen ];
	char		errorStr[ 64 ];
	
	Unused( inSDRef );
	Unused( inFlags );
	Unused( inContext );
	
	GetTimestampStr( time );
	
	if( inError ) SNPrintF( errorStr, sizeof( errorStr ), " error %#m", inError );
	
	FPrintF( stdout, "%s: %s can be reached at %s:%u (interface %d)%?s\n",
		time, inFullName, inHostname, ntohs( inPort ), (int32_t) inInterfaceIndex, inError, errorStr );
	if( inTXTLen == 1 )
	{
		FPrintF( stdout, " TXT record: %#H\n", inTXTPtr, (int) inTXTLen, INT_MAX );
	}
	else
	{
		FPrintF( stdout, " TXT record: %#{txt}\n", inTXTPtr, (size_t) inTXTLen );
	}
}

//===========================================================================================================================
//	GetAddrInfoPOSIXCmd
//===========================================================================================================================

#define AddressFamilyStr( X ) (				\
	( (X) == AF_INET )		? "inet"	:	\
	( (X) == AF_INET6 )		? "inet6"	:	\
	( (X) == AF_UNSPEC )	? "unspec"	:	\
							  "???" )

static void	GetAddrInfoPOSIXCmd( void )
{
	OSStatus				err;
	struct addrinfo			hints;
	struct addrinfo *		addrInfo;
	struct addrinfo *		addrInfoList = NULL;
	int						addrCount;
	char					time[ kTimestampBufLen ];
	
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_socktype = SOCK_STREAM;
	if( !gGAIPOSIX_Family )										hints.ai_family = AF_UNSPEC;
	else if( strcasecmp( gGAIPOSIX_Family, "inet" ) == 0 )		hints.ai_family = AF_INET;
	else if( strcasecmp( gGAIPOSIX_Family, "inet6" ) == 0 )		hints.ai_family = AF_INET6;
	else if( strcasecmp( gGAIPOSIX_Family, "unspec" ) == 0 )	hints.ai_family = AF_UNSPEC;
	else
	{
		FPrintF( stderr, "Invalid address family: %s.\n", gGAIPOSIX_Family );
		err = kParamErr;
		goto exit;
	}
	
	FPrintF( stdout, "Name:           %s\n", gGAIPOSIX_Name );
	FPrintF( stdout, "Address family: %s\n", AddressFamilyStr( hints.ai_family ) );
	FPrintF( stdout, "Start time:     %s\n", GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
	
	err = getaddrinfo( gGAIPOSIX_Name, NULL, &hints, &addrInfoList );
	FPrintF( stdout, "%s:\n", GetTimestampStr( time ) );
	if( err )
	{
		FPrintF( stderr, "getaddrinfo() error %#m: %s.\n", err, gai_strerror( err ) );
		goto exit;
	}
	
	addrCount = 0;
	for( addrInfo = addrInfoList; addrInfo; addrInfo = addrInfo->ai_next ) { ++addrCount; }
	
	FPrintF( stdout, "Addresses (%d total):\n", addrCount );
	for( addrInfo = addrInfoList; addrInfo; addrInfo = addrInfo->ai_next )
	{
		FPrintF( stdout, "%##a\n", addrInfo->ai_addr );
	}
	
exit:
	FPrintF( stdout, "---\n" );
	FPrintF( stdout, "End time:       %s\n", GetTimestampStr( time ) );
	
	if( addrInfoList ) freeaddrinfo( addrInfoList );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	ReverseLookupCmd
//===========================================================================================================================

static void	ReverseLookupCmd( void )
{
	OSStatus					err;
	QueryRecordContext *		context			= NULL;
	DNSServiceRef				sdRef;
	dispatch_source_t			signalSource	= NULL;
	uint32_t					ipv4Addr;
	uint8_t						ipv6Addr[ 16 ];
	char						recordName[ ( 16 * 4 ) + 9 + 1 ];
	int							useMainConnection;
	
	// Set up SIGINT handler.
	
	signal( SIGINT, SIG_IGN );
	err = DispatchSignalSourceCreate( SIGINT, Exit, kExitReason_SIGINT, &signalSource );
	require_noerr( err, exit );
	dispatch_resume( signalSource );
	
	// Create context.
	
	context = (QueryRecordContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	// Check command parameters.
	
	if( gReverseLookup_TimeLimitSecs < 0 )
	{
		FPrintF( stderr, "Invalid time limit: %d s.\n", gReverseLookup_TimeLimitSecs );
		err = kParamErr;
		goto exit;
	}
	
	// Create main connection.
	
	if( gConnectionOpt )
	{
		err = CreateConnectionFromArgString( gConnectionOpt, dispatch_get_main_queue(), &context->mainRef, NULL );
		require_noerr_quiet( err, exit );
		useMainConnection = true;
	}
	else
	{
		useMainConnection = false;
	}
	
	// Get flags.
	
	context->flags = GetDNSSDFlagsFromOpts();
	if( useMainConnection ) context->flags |= kDNSServiceFlagsShareConnection;
	
	// Get interface index.
	
	err = InterfaceIndexFromArgString( gInterface, &context->ifIndex );
	require_noerr_quiet( err, exit );
	
	// Create reverse lookup record name.
	
	err = StringToIPv4Address( gReverseLookup_IPAddr, kStringToIPAddressFlagsNoPort | kStringToIPAddressFlagsNoPrefix,
		&ipv4Addr, NULL, NULL, NULL, NULL );
	if( err )
	{
		char *		dst;
		int			i;
		
		err = StringToIPv6Address( gReverseLookup_IPAddr,
			kStringToIPAddressFlagsNoPort | kStringToIPAddressFlagsNoPrefix | kStringToIPAddressFlagsNoScope,
			ipv6Addr, NULL, NULL, NULL, NULL );
		if( err )
		{
			FPrintF( stderr, "Invalid IP address: \"%s\".\n", gReverseLookup_IPAddr );
			err = kParamErr;
			goto exit;
		}
		dst = recordName;
		for( i = 15; i >= 0; --i )
		{
			*dst++ = kHexDigitsLowercase[ ipv6Addr[ i ] & 0x0F ];
			*dst++ = '.';
			*dst++ = kHexDigitsLowercase[ ipv6Addr[ i ] >> 4 ];
			*dst++ = '.';
		}
		strcpy( dst, "ip6.arpa." );
		check( ( strlen( recordName ) + 1 ) <= sizeof( recordName ) );
	}
	else
	{
		SNPrintF( recordName, sizeof( recordName ), "%u.%u.%u.%u.in-addr.arpa.",
			  ipv4Addr         & 0xFF,
			( ipv4Addr >>  8 ) & 0xFF,
			( ipv4Addr >> 16 ) & 0xFF,
			( ipv4Addr >> 24 ) & 0xFF );
	}
	
	// Set remaining parameters.
	
	context->recordName		= recordName;
	context->recordType		= kDNSServiceType_PTR;
	context->timeLimitSecs	= gReverseLookup_TimeLimitSecs;
	context->oneShotMode	= gReverseLookup_OneShot ? true : false;
	
	// Print prologue.
	
	QueryRecordPrintPrologue( context );
	
	// Start operation.
	
	if( useMainConnection ) sdRef = context->mainRef;
	err = DNSServiceQueryRecord( &sdRef, context->flags, context->ifIndex, context->recordName, context->recordType,
		kDNSServiceClass_IN, QueryRecordCallback, context );
	require_noerr( err, exit );
	
	context->opRef = sdRef;
	if( !useMainConnection )
	{
		err = DNSServiceSetDispatchQueue( context->opRef, dispatch_get_main_queue() );
		require_noerr( err, exit );
	}
	
	// Set time limit.
	
	if( context->timeLimitSecs > 0 )
	{
		dispatch_after_f( dispatch_time_seconds( context->timeLimitSecs ), dispatch_get_main_queue(),
			kExitReason_TimeLimit, Exit );
	}
	dispatch_main();
	
exit:
	dispatch_source_forget( &signalSource );
	if( context ) QueryRecordContextFree( context );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	BrowseAllCmd
//===========================================================================================================================

typedef struct BrowseDomain			BrowseDomain;
typedef struct BrowseType			BrowseType;
typedef struct BrowseOp				BrowseOp;
typedef struct BrowseInstance		BrowseInstance;
typedef struct BrowseIPAddr			BrowseIPAddr;

typedef struct
{
	int						refCount;
	DNSServiceRef			mainRef;
	DNSServiceRef			domainsQuery;
	const char *			domain;
	BrowseDomain *			domainList;
	char **					serviceTypes;
	size_t					serviceTypesCount;
	dispatch_source_t		exitTimer;
	uint32_t				ifIndex;
	int						pendingConnectCount;
	int						browseTimeSecs;
	int						connectTimeLimitSecs;
	Boolean					includeAWDL;
	Boolean					useColoredText;
	
}	BrowseAllContext;

struct BrowseDomain
{
	BrowseDomain *			next;
	char *					name;
	DNSServiceRef			servicesQuery;
	BrowseAllContext *		context;
	BrowseType *			typeList;
};

struct BrowseType
{
	BrowseType *		next;
	char *				name;
	BrowseOp *			browseList;
};

struct BrowseOp
{
	BrowseOp *				next;
	BrowseAllContext *		context;
	DNSServiceRef			browse;
	uint64_t				startTicks;
	BrowseInstance *		instanceList;
	uint32_t				ifIndex;
	Boolean					isTCP;
};

struct BrowseInstance
{
	BrowseInstance *		next;
	BrowseAllContext *		context;
	char *					name;
	uint64_t				foundTicks;
	DNSServiceRef			resolve;
	uint64_t				resolveStartTicks;
	uint64_t				resolveDoneTicks;
	DNSServiceRef			getAddr;
	uint64_t				getAddrStartTicks;
	BrowseIPAddr *			addrList;
	uint8_t *				txtPtr;
	size_t					txtLen;
	char *					hostname;
	uint32_t				ifIndex;
	uint16_t				port;
	Boolean					isTCP;
};

typedef enum
{
	kConnectStatus_None			= 0,
	kConnectStatus_Pending		= 1,
	kConnectStatus_Succeeded	= 2,
	kConnectStatus_Failed		= 3
	
}	ConnectStatus;

struct BrowseIPAddr
{
	BrowseIPAddr *			next;
	sockaddr_ip				sip;
	int						refCount;
	BrowseAllContext *		context;
	uint64_t				foundTicks;
	AsyncConnectionRef		connection;
	ConnectStatus			connectStatus;
	CFTimeInterval			connectTimeSecs;
	OSStatus				connectError;
};

static void	BrowseAllPrintPrologue( const BrowseAllContext *inContext );
static void DNSSD_API
	BrowseAllQueryDomainsCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		uint16_t				inType,
		uint16_t				inClass,
		uint16_t				inRDataLen,
		const void *			inRDataPtr,
		uint32_t				inTTL,
		void *					inContext );
static void DNSSD_API
	BrowseAllQueryCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		uint16_t				inType,
		uint16_t				inClass,
		uint16_t				inRDataLen,
		const void *			inRDataPtr,
		uint32_t				inTTL,
		void *					inContext );
static void DNSSD_API
	BrowseAllBrowseCallback(
		DNSServiceRef		inSDRef,
		DNSServiceFlags		inFlags,
		uint32_t			inInterfaceIndex,
		DNSServiceErrorType	inError,
		const char *		inName,
		const char *		inRegType,
		const char *		inDomain,
		void *				inContext );
static void DNSSD_API
	BrowseAllResolveCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		const char *			inHostname,
		uint16_t				inPort,
		uint16_t				inTXTLen,
		const unsigned char *	inTXTPtr,
		void *					inContext );
static void DNSSD_API
	BrowseAllGAICallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inHostname,
		const struct sockaddr *	inSockAddr,
		uint32_t				inTTL,
		void *					inContext );
static void		BrowseAllConnectionProgress( int inPhase, const void *inDetails, void *inArg );
static void		BrowseAllConnectionHandler( SocketRef inSock, OSStatus inError, void *inArg );
static void		BrowseAllStop( void *inContext );
static void		BrowseAllExit( void *inContext );
static OSStatus	BrowseAllAddDomain( BrowseAllContext *inContext, const char *inName );
static OSStatus	BrowseAllRemoveDomain( BrowseAllContext *inContext, const char *inName );
static void		BrowseAllContextRelease( BrowseAllContext *inContext );
static OSStatus
	BrowseAllAddServiceType(
		BrowseAllContext *	inContext,
		BrowseDomain *		inDomain,
		const char *		inName,
		uint32_t			inIfIndex,
		Boolean				inIncludeAWDL );
static OSStatus
	BrowseAllRemoveServiceType(
		BrowseAllContext *	inContext,
		BrowseDomain *		inDomain,
		const char *		inName,
		uint32_t			inIfIndex );
static OSStatus
	BrowseAllAddServiceInstance(
		BrowseAllContext *	inContext,
		BrowseOp *			inBrowse,
		const char *		inName,
		const char *		inRegType,
		const char *		inDomain,
		uint32_t			inIfIndex );
static OSStatus
	BrowseAllRemoveServiceInstance(
		BrowseAllContext *	inContext,
		BrowseOp *			inBrowse,
		const char *		inName,
		uint32_t			inIfIndex );
static OSStatus
	BrowseAllAddIPAddress(
		BrowseAllContext *		inContext,
		BrowseInstance *		inInstance,
		const struct sockaddr *	inSockAddr );
static OSStatus
	BrowseAllRemoveIPAddress(
		BrowseAllContext *		inContext,
		BrowseInstance *		inInstance,
		const struct sockaddr *	inSockAddr );
static void	BrowseDomainFree( BrowseDomain *inDomain );
static void	BrowseTypeFree( BrowseType *inType );
static void	BrowseOpFree( BrowseOp *inBrowse );
static void	BrowseInstanceFree( BrowseInstance *inInstance );
static void	BrowseIPAddrRelease( BrowseIPAddr *inAddr );
static void	BrowseIPAddrReleaseList( BrowseIPAddr *inList );

#define ForgetIPAddressList( X )		ForgetCustom( X, BrowseIPAddrReleaseList )
#define ForgetBrowseAllContext( X )		ForgetCustom( X, BrowseAllContextRelease )

#define kBrowseAllOpenFileMin		4096

static void	BrowseAllCmd( void )
{
	OSStatus				err;
	BrowseAllContext *		context = NULL;
	
	// Check command parameters.
	
	if( gBrowseAll_BrowseTimeSecs <= 0 )
	{
		FPrintF( stdout, "Invalid browse time: %d seconds.\n", gBrowseAll_BrowseTimeSecs );
		err = kParamErr;
		goto exit;
	}
	
#if( TARGET_OS_POSIX )
	// Set open file minimum.
	
	{
		struct rlimit		fdLimits;
		
		err = getrlimit( RLIMIT_NOFILE, &fdLimits );
		err = map_global_noerr_errno( err );
		require_noerr( err, exit );
		
		if( fdLimits.rlim_cur < kBrowseAllOpenFileMin )
		{
			fdLimits.rlim_cur = kBrowseAllOpenFileMin;
			err = setrlimit( RLIMIT_NOFILE, &fdLimits );
			err = map_global_noerr_errno( err );
			require_noerr( err, exit );
		}
	}
#endif
	
	context = (BrowseAllContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	context->refCount				= 1;
	context->domain					= gBrowseAll_Domain;
	context->serviceTypes			= gBrowseAll_ServiceTypes;
	context->serviceTypesCount		= gBrowseAll_ServiceTypesCount;
	gBrowseAll_ServiceTypes			= NULL;
	gBrowseAll_ServiceTypesCount	= 0;
	context->browseTimeSecs			= gBrowseAll_BrowseTimeSecs;
	context->connectTimeLimitSecs	= gBrowseAll_ConnectTimeLimitSecs;
	context->includeAWDL			= gBrowseAll_IncludeAWDL ? true : false;
#if( TARGET_OS_POSIX )
	context->useColoredText			= isatty( STDOUT_FILENO ) ? true : false;
#endif
	
	err = DNSServiceCreateConnection( &context->mainRef );
	require_noerr( err, exit );
	
	err = DNSServiceSetDispatchQueue( context->mainRef, dispatch_get_main_queue() );
	require_noerr( err, exit );
	
	// Set interface index.
	
	err = InterfaceIndexFromArgString( gInterface, &context->ifIndex );
	require_noerr_quiet( err, exit );
	
	BrowseAllPrintPrologue( context );
	
	if( context->domain )
	{
		err = BrowseAllAddDomain( context, context->domain );
		require_noerr( err, exit );
	}
	else
	{
		DNSServiceRef		sdRef;
		
		sdRef = context->mainRef;
		err = DNSServiceQueryRecord( &sdRef, kDNSServiceFlagsShareConnection, kDNSServiceInterfaceIndexLocalOnly,
			"b._dns-sd._udp.local.", kDNSServiceType_PTR, kDNSServiceClass_IN, BrowseAllQueryDomainsCallback, context );
		require_noerr( err, exit );
		
		context->domainsQuery = sdRef;
	}
	
	dispatch_after_f( dispatch_time_seconds( context->browseTimeSecs ), dispatch_get_main_queue(), context, BrowseAllStop );
	dispatch_main();
	
exit:
	if( context ) BrowseAllContextRelease( context );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	BrowseAllPrintPrologue
//===========================================================================================================================

static void	BrowseAllPrintPrologue( const BrowseAllContext *inContext )
{
	size_t		i;
	char		ifName[ kInterfaceNameBufLen ];
	char		time[ kTimestampBufLen ];
	
	InterfaceIndexToName( inContext->ifIndex, ifName );
	
	FPrintF( stdout, "Interface:          %d (%s)\n",	(int32_t) inContext->ifIndex, ifName );
	FPrintF( stdout, "Service types:      ");
	if( inContext->serviceTypesCount > 0 )
	{
		FPrintF( stdout, "%s", inContext->serviceTypes[ 0 ] );
		for( i = 1; i < inContext->serviceTypesCount; ++i ) FPrintF( stdout, ", %s", inContext->serviceTypes[ i ] );
		FPrintF( stdout, "\n" );
	}
	else
	{
		FPrintF( stdout, "all services\n" );
	}
	FPrintF( stdout, "Domain:             %s\n", inContext->domain ? inContext->domain : "default domains" );
	FPrintF( stdout, "Browse time:        %d second%?c\n", inContext->browseTimeSecs, inContext->browseTimeSecs != 1, 's' );
	FPrintF( stdout, "Connect time limit: %d second%?c\n",
		inContext->connectTimeLimitSecs, inContext->connectTimeLimitSecs != 1, 's' );
	FPrintF( stdout, "Start time:         %s\n", GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
}

//===========================================================================================================================
//	BrowseAllQueryDomainsCallback
//===========================================================================================================================

static void DNSSD_API
	BrowseAllQueryDomainsCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		uint16_t				inType,
		uint16_t				inClass,
		uint16_t				inRDataLen,
		const void *			inRDataPtr,
		uint32_t				inTTL,
		void *					inContext )
{
	OSStatus						err;
	BrowseAllContext * const		context = (BrowseAllContext *) inContext;
	char							domainStr[ kDNSServiceMaxDomainName ];
	
	Unused( inSDRef );
	Unused( inInterfaceIndex );
	Unused( inFullName );
	Unused( inType );
	Unused( inClass );
	Unused( inTTL );
	
	err = inError;
	require_noerr( err, exit );
	
	err = DomainNameToString( inRDataPtr, ( (const uint8_t *) inRDataPtr ) + inRDataLen, domainStr, NULL );
	require_noerr( err, exit );
	
	if( inFlags & kDNSServiceFlagsAdd )
	{
		err = BrowseAllAddDomain( context, domainStr );
		if( err == kDuplicateErr ) err = kNoErr;
		require_noerr( err, exit );
	}
	else
	{
		err = BrowseAllRemoveDomain( context, domainStr );
		if( err == kNotFoundErr ) err = kNoErr;
		require_noerr( err, exit );
	}
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	BrowseAllQueryCallback
//===========================================================================================================================

static void DNSSD_API
	BrowseAllQueryCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		uint16_t				inType,
		uint16_t				inClass,
		uint16_t				inRDataLen,
		const void *			inRDataPtr,
		uint32_t				inTTL,
		void *					inContext )
{
	OSStatus					err;
	BrowseDomain * const		domain			= (BrowseDomain *) inContext;
	const uint8_t *				firstLabel;
	const uint8_t *				secondLabel;
	char *						serviceTypeStr	= NULL;
	const uint8_t * const		end				= ( (uint8_t * ) inRDataPtr ) + inRDataLen;
	
	Unused( inSDRef );
	Unused( inFullName );
	Unused( inTTL );
	Unused( inType );
	Unused( inClass );
	
	err = inError;
	require_noerr( err, exit );
	
	check( inType	== kDNSServiceType_PTR );
	check( inClass	== kDNSServiceClass_IN );
	require_action( inRDataLen > 0, exit, err = kSizeErr );
	
	firstLabel = inRDataPtr;
	require_action_quiet( ( firstLabel + 1 + firstLabel[ 0 ] ) < end , exit, err = kUnderrunErr );
	
	secondLabel = firstLabel + 1 + firstLabel[ 0 ];
	require_action_quiet( ( secondLabel + 1 + secondLabel[ 0 ] ) < end , exit, err = kUnderrunErr );
	
	ASPrintF( &serviceTypeStr, "%#s.%#s", firstLabel, secondLabel );
	require_action( serviceTypeStr, exit, err = kNoMemoryErr );
	
	if( inFlags & kDNSServiceFlagsAdd )
	{
		err = BrowseAllAddServiceType( domain->context, domain, serviceTypeStr, inInterfaceIndex, false );
		if( err == kDuplicateErr ) err = kNoErr;
		require_noerr( err, exit );
	}
	else
	{
		err = BrowseAllRemoveServiceType( domain->context, domain, serviceTypeStr, inInterfaceIndex );
		if( err == kNotFoundErr ) err = kNoErr;
		require_noerr( err, exit );
	}
	
exit:
	FreeNullSafe( serviceTypeStr );
}

//===========================================================================================================================
//	BrowseAllBrowseCallback
//===========================================================================================================================

static void DNSSD_API
	BrowseAllBrowseCallback(
		DNSServiceRef		inSDRef,
		DNSServiceFlags		inFlags,
		uint32_t			inInterfaceIndex,
		DNSServiceErrorType	inError,
		const char *		inName,
		const char *		inRegType,
		const char *		inDomain,
		void *				inContext )
{
	OSStatus				err;
	BrowseOp * const		browse = (BrowseOp *) inContext;
	
	Unused( inSDRef );
	
	err = inError;
	require_noerr( err, exit );
	
	if( inFlags & kDNSServiceFlagsAdd )
	{
		err = BrowseAllAddServiceInstance( browse->context, browse, inName, inRegType, inDomain, inInterfaceIndex );
		if( err == kDuplicateErr ) err = kNoErr;
		require_noerr( err, exit );
	}
	else
	{
		err = BrowseAllRemoveServiceInstance( browse->context, browse, inName, inInterfaceIndex );
		if( err == kNotFoundErr ) err = kNoErr;
		require_noerr( err, exit );
	}
	
exit:
	return;
}

//===========================================================================================================================
//	BrowseAllResolveCallback
//===========================================================================================================================

static void DNSSD_API
	BrowseAllResolveCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inFullName,
		const char *			inHostname,
		uint16_t				inPort,
		uint16_t				inTXTLen,
		const unsigned char *	inTXTPtr,
		void *					inContext )
{
	OSStatus					err;
	const uint64_t				nowTicks	= UpTicks();
	BrowseInstance * const		instance	= (BrowseInstance *) inContext;
	
	Unused( inSDRef );
	Unused( inFlags );
	Unused( inInterfaceIndex );
	Unused( inFullName );
	
	err = inError;
	require_noerr( err, exit );
	
	if( !MemEqual( instance->txtPtr, instance->txtLen, inTXTPtr, inTXTLen ) )
	{
		FreeNullSafe( instance->txtPtr );
		instance->txtPtr = malloc( inTXTLen );
		require_action( instance->txtPtr, exit, err = kNoMemoryErr );
		
		memcpy( instance->txtPtr, inTXTPtr, inTXTLen );
		instance->txtLen = inTXTLen;
	}
	
	instance->port = ntohs( inPort );
	
	if( !instance->hostname || ( strcasecmp( instance->hostname, inHostname ) != 0 ) )
	{
		DNSServiceRef		sdRef;
		
		if( !instance->hostname ) instance->resolveDoneTicks = nowTicks;
		FreeNullSafe( instance->hostname );
		instance->hostname = strdup( inHostname );
		require_action( instance->hostname, exit, err = kNoMemoryErr );
		
		DNSServiceForget( &instance->getAddr );
		ForgetIPAddressList( &instance->addrList );
		
		sdRef = instance->context->mainRef;
		instance->getAddrStartTicks = UpTicks();
		err = DNSServiceGetAddrInfo( &sdRef, kDNSServiceFlagsShareConnection, instance->ifIndex,
			kDNSServiceProtocol_IPv4 | kDNSServiceProtocol_IPv6, instance->hostname, BrowseAllGAICallback, instance );
		require_noerr( err, exit );
		
		instance->getAddr = sdRef;
	}
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	BrowseAllGAICallback
//===========================================================================================================================

static void DNSSD_API
	BrowseAllGAICallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inHostname,
		const struct sockaddr *	inSockAddr,
		uint32_t				inTTL,
		void *					inContext )
{
	OSStatus					err;
	BrowseInstance * const		instance = (BrowseInstance *) inContext;
	
	Unused( inSDRef );
	Unused( inInterfaceIndex );
	Unused( inHostname );
	Unused( inTTL );
	
	err = inError;
	require_noerr( err, exit );
	
	if( ( inSockAddr->sa_family != AF_INET ) && ( inSockAddr->sa_family != AF_INET6 ) )
	{
		dlogassert( "Unexpected address family: %d", inSockAddr->sa_family );
		goto exit;
	}
	
	if( inFlags & kDNSServiceFlagsAdd )
	{
		err = BrowseAllAddIPAddress( instance->context, instance, inSockAddr );
		if( err == kDuplicateErr ) err = kNoErr;
		require_noerr( err, exit );
	}
	else
	{
		err = BrowseAllRemoveIPAddress( instance->context, instance, inSockAddr );
		if( err == kNotFoundErr ) err = kNoErr;
		require_noerr( err, exit );
	}
	
exit:
	return;
}

//===========================================================================================================================
//	BrowseAllConnectionProgress
//===========================================================================================================================

static void	BrowseAllConnectionProgress( int inPhase, const void *inDetails, void *inArg )
{
	BrowseIPAddr * const		addr = (BrowseIPAddr *) inArg;
	
	if( inPhase == kAsyncConnectionPhase_Connected )
	{
		const AsyncConnectedInfo * const		info = (AsyncConnectedInfo *) inDetails;
		
		addr->connectTimeSecs = info->connectSecs;
	}
}

//===========================================================================================================================
//	BrowseAllConnectionHandler
//===========================================================================================================================

static void	BrowseAllConnectionHandler( SocketRef inSock, OSStatus inError, void *inArg )
{
	BrowseIPAddr * const			addr	= (BrowseIPAddr *) inArg;
	BrowseAllContext * const		context	= addr->context;
	
	if( inError )
	{
		addr->connectStatus	= kConnectStatus_Failed;
		addr->connectError	= inError;
	}
	else
	{
		addr->connectStatus = kConnectStatus_Succeeded;
	}
	
	check( context->pendingConnectCount > 0 );
	if( --context->pendingConnectCount == 0 )
	{
		if( context->exitTimer )
		{
			dispatch_source_forget( &context->exitTimer );
			dispatch_async_f( dispatch_get_main_queue(), context, BrowseAllExit );
		}
	}
	
	ForgetSocket( &inSock );
	BrowseIPAddrRelease( addr );
}

//===========================================================================================================================
//	BrowseAllStop
//===========================================================================================================================

static void	BrowseAllStop( void *inContext )
{
	OSStatus						err;
	BrowseAllContext * const		context = (BrowseAllContext *) inContext;
	BrowseDomain *					domain;
	BrowseType *					type;
	BrowseOp *						browse;
	BrowseInstance *				instance;
	
	DNSServiceForget( &context->domainsQuery );
	for( domain = context->domainList; domain; domain = domain->next )
	{
		DNSServiceForget( &domain->servicesQuery );
		for( type = domain->typeList; type; type = type->next )
		{
			for( browse = type->browseList; browse; browse = browse->next )
			{
				DNSServiceForget( &browse->browse );
				for( instance = browse->instanceList; instance; instance = instance->next )
				{
					DNSServiceForget( &instance->resolve );
					DNSServiceForget( &instance->getAddr );
				}
			}
		}
	}
	DNSServiceForget( &context->mainRef );
	
	if( ( context->pendingConnectCount > 0 ) && ( context->connectTimeLimitSecs > 0 ) )
	{
		check( !context->exitTimer );
		err = DispatchTimerCreate( dispatch_time_seconds( context->connectTimeLimitSecs ), DISPATCH_TIME_FOREVER,
			100 * kNanosecondsPerMillisecond, BrowseAllExit, NULL, context, &context->exitTimer );
		require_noerr( err, exit );
		dispatch_resume( context->exitTimer );
	}
	else
	{
		dispatch_async_f( dispatch_get_main_queue(), context, BrowseAllExit );
	}
	err = kNoErr;
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	BrowseAllExit
//===========================================================================================================================

#define kStatusStr_CouldConnect					"connected"
#define kStatusStr_CouldConnectColored			kANSIGreen kStatusStr_CouldConnect kANSINormal
#define kStatusStr_CouldNotConnect				"could not connect"
#define kStatusStr_CouldNotConnectColored		kANSIRed kStatusStr_CouldNotConnect kANSINormal
#define kStatusStr_NoConnectionAttempted		"no connection attempted"
#define kStatusStr_Unknown						"unknown"

#define Indent( X )		( (X) * 4 ), ""

static void	BrowseAllExit( void *inContext )
{
	BrowseAllContext * const		context = (BrowseAllContext *) inContext;
	BrowseDomain *					domain;
	BrowseType *					type;
	BrowseOp *						browse;
	BrowseInstance *				instance;
	BrowseIPAddr *					addr;
	
	dispatch_source_forget( &context->exitTimer );
	
	for( domain = context->domainList; domain; domain = domain->next )
	{
		FPrintF( stdout, "%s\n\n", domain->name );
		
		for( type = domain->typeList; type; type = type->next )
		{
			const char *		desc;
			
			desc = ServiceTypeDescription( type->name );
			if( desc )	FPrintF( stdout, "%*s" "%s (%s)\n\n",	Indent( 1 ), desc, type->name );
			else		FPrintF( stdout, "%*s" "%s\n\n",		Indent( 1 ), type->name );
			
			for( browse = type->browseList; browse; browse = browse->next )
			{
				for( instance = browse->instanceList; instance; instance = instance->next )
				{
					char		ifname[ IF_NAMESIZE + 1 ];
					
					FPrintF( stdout, "%*s" "%s via ", Indent( 2 ), instance->name );
					FPrintF( stdout, "%*s" "%s via ", Indent( 2 ), instance->name );
					if( instance->ifIndex == 0 )
					{
						FPrintF( stdout, "the Internet" );
					}
					else if( if_indextoname( instance->ifIndex, ifname ) )
					{
						NetTransportType		netType;
						
						SocketGetInterfaceInfo( kInvalidSocketRef, ifname, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
							&netType );
						FPrintF( stdout, "%s (%s)",
							( netType == kNetTransportType_Ethernet ) ? "ethernet" : NetTransportTypeToString( netType ),
							ifname );
					}
					else
					{
						FPrintF( stdout, "interface index %u", instance->ifIndex );
					}
					FPrintF( stdout, "\n\n" );
					
					if( instance->hostname )
					{
						char		buffer[ 256 ];
						
						SNPrintF( buffer, sizeof( buffer ), "%s:%u", instance->hostname, instance->port );
						FPrintF( stdout, "%*s" "%-51s %4llu ms\n", Indent( 3 ), buffer,
							UpTicksToMilliseconds( instance->resolveDoneTicks - instance->resolveStartTicks ) );
					}
					else
					{
						FPrintF( stdout, "%*s" "%s:%u\n", Indent( 3 ), instance->hostname, instance->port );
					}
					
					for( addr = instance->addrList; addr; addr = addr->next )
					{
						AsyncConnection_Forget( &addr->connection );
						
						if( addr->connectStatus == kConnectStatus_Pending )
						{
							addr->connectStatus	= kConnectStatus_Failed;
							addr->connectError	= kTimeoutErr;
						}
						
						FPrintF( stdout, "%*s" "%-##47a %4llu ms (", Indent( 4 ),
							&addr->sip.sa, UpTicksToMilliseconds( addr->foundTicks - instance->getAddrStartTicks ) );
						switch( addr->connectStatus )
						{
							case kConnectStatus_None:
								FPrintF( stdout, "%s", kStatusStr_NoConnectionAttempted );
								break;
							
							case kConnectStatus_Succeeded:
								FPrintF( stdout, "%s in %.2f ms",
									context->useColoredText ? kStatusStr_CouldConnectColored : kStatusStr_CouldConnect,
									addr->connectTimeSecs * 1000 );
								break;
							
							case kConnectStatus_Failed:
								FPrintF( stdout, "%s: %m",
									context->useColoredText ? kStatusStr_CouldNotConnectColored : kStatusStr_CouldNotConnect,
									addr->connectError );
								break;
							
							default:
								FPrintF( stdout, "%s", kStatusStr_Unknown );
								break;
						}
						FPrintF( stdout, ")\n" );
					}
					
					FPrintF( stdout, "\n" );
					if( instance->txtLen == 0 ) continue;
					
					FPrintF( stdout, "%*s" "TXT record:\n", Indent( 3 ) );
					if( instance->txtLen > 1 )
					{
						FPrintF( stdout, "%3{txt}", instance->txtPtr, instance->txtLen );
					}
					else
					{
						FPrintF( stdout, "%*s" "%#H\n", Indent( 3 ), instance->txtPtr, (int) instance->txtLen, INT_MAX );
					}
					FPrintF( stdout, "\n" );
				}
			}
			FPrintF( stdout, "\n" );
		}
	}
	
	while( ( domain = context->domainList ) != NULL )
	{
		context->domainList = domain->next;
		BrowseDomainFree( domain );
	}
	
	BrowseAllContextRelease( context );
	Exit( NULL );
}

//===========================================================================================================================
//	BrowseAllAddDomain
//===========================================================================================================================

static OSStatus	BrowseAllAddDomain( BrowseAllContext *inContext, const char *inName )
{
	OSStatus			err;
	BrowseDomain *		domain;
	BrowseDomain **		p;
	BrowseDomain *		newDomain = NULL;
	
	for( p = &inContext->domainList; ( domain = *p ) != NULL; p = &domain->next )
	{
		if( strcasecmp( domain->name, inName ) == 0 ) break;
	}
	require_action_quiet( !domain, exit, err = kDuplicateErr );
	
	newDomain = (BrowseDomain *) calloc( 1, sizeof( *newDomain ) );
	require_action( newDomain, exit, err = kNoMemoryErr );
	
	++inContext->refCount;
	newDomain->context = inContext;
	
	newDomain->name = strdup( inName );
	require_action( newDomain->name, exit, err = kNoMemoryErr );
	
	if( inContext->serviceTypesCount > 0 )
	{
		size_t		i;
		
		for( i = 0; i < inContext->serviceTypesCount; ++i )
		{
			err = BrowseAllAddServiceType( inContext, newDomain, inContext->serviceTypes[ i ], inContext->ifIndex,
				inContext->includeAWDL );
			if( err == kDuplicateErr ) err = kNoErr;
			require_noerr( err, exit );
		}
	}
	else
	{
		char *				recordName;
		DNSServiceFlags		flags;
		DNSServiceRef		sdRef;
		
		ASPrintF( &recordName, "_services._dns-sd._udp.%s", newDomain->name );
		require_action( recordName, exit, err = kNoMemoryErr );
		
		flags = kDNSServiceFlagsShareConnection;
		if( inContext->includeAWDL ) flags |= kDNSServiceFlagsIncludeAWDL;
		
		sdRef = newDomain->context->mainRef;
		err = DNSServiceQueryRecord( &sdRef, flags, inContext->ifIndex, recordName, kDNSServiceType_PTR, kDNSServiceClass_IN,
			BrowseAllQueryCallback, newDomain );
		free( recordName );
		require_noerr( err, exit );
		
		newDomain->servicesQuery = sdRef;
	}
	
	*p = newDomain;
	newDomain = NULL;
	err = kNoErr;
	
exit:
	if( newDomain ) BrowseDomainFree( newDomain );
	return( err );
}

//===========================================================================================================================
//	BrowseAllRemoveDomain
//===========================================================================================================================

static OSStatus	BrowseAllRemoveDomain( BrowseAllContext *inContext, const char *inName )
{
	OSStatus			err;
	BrowseDomain *		domain;
	BrowseDomain **		p;
	
	for( p = &inContext->domainList; ( domain = *p ) != NULL; p = &domain->next )
	{
		if( strcasecmp( domain->name, inName ) == 0 ) break;
	}
	
	if( domain )
	{
		*p = domain->next;
		BrowseDomainFree( domain );
		err = kNoErr;
	}
	else
	{
		err = kNotFoundErr;
	}
	
	return( err );
}

//===========================================================================================================================
//	BrowseAllContextRelease
//===========================================================================================================================

static void	BrowseAllContextRelease( BrowseAllContext *inContext )
{
	if( --inContext->refCount == 0 )
	{
		check( !inContext->domainsQuery );
		check( !inContext->domainList );
		check( !inContext->exitTimer );
		check( !inContext->pendingConnectCount );
		DNSServiceForget( &inContext->mainRef );
		if( inContext->serviceTypes )
		{
			StringArray_Free( inContext->serviceTypes, inContext->serviceTypesCount );
			inContext->serviceTypes			= NULL;
			inContext->serviceTypesCount	= 0;
		}
		free( inContext );
	}
}

//===========================================================================================================================
//	BrowseAllAddServiceType
//===========================================================================================================================

static OSStatus
	BrowseAllAddServiceType(
		BrowseAllContext *	inContext,
		BrowseDomain *		inDomain,
		const char *		inName,
		uint32_t			inIfIndex,
		Boolean				inIncludeAWDL )
{
	OSStatus			err;
	DNSServiceRef		sdRef;
	DNSServiceFlags		flags;
	BrowseType *		type;
	BrowseType **		typePtr;
	BrowseType *		newType		= NULL;
	BrowseOp *			browse;
	BrowseOp **			browsePtr;
	BrowseOp *			newBrowse	= NULL;
	
	for( typePtr = &inDomain->typeList; ( type = *typePtr ) != NULL; typePtr = &type->next )
	{
		if( strcasecmp( type->name, inName ) == 0 ) break;
	}
	if( !type )
	{
		newType = (BrowseType *) calloc( 1, sizeof( *newType ) );
		require_action( newType, exit, err = kNoMemoryErr );
		
		newType->name = strdup( inName );
		require_action( newType->name, exit, err = kNoMemoryErr );
		
		type = newType;
	}
	
	for( browsePtr = &type->browseList; ( browse = *browsePtr ) != NULL; browsePtr = &browse->next )
	{
		if( browse->ifIndex == inIfIndex ) break;
	}
	require_action_quiet( !browse, exit, err = kDuplicateErr );
	
	newBrowse = (BrowseOp *) calloc( 1, sizeof( *newBrowse ) );
	require_action( newBrowse, exit, err = kNoMemoryErr );
	
	++inContext->refCount;
	newBrowse->context	= inContext;
	newBrowse->ifIndex	= inIfIndex;
	if( stricmp_suffix( inName, "._tcp" ) == 0 ) newBrowse->isTCP = true;
	
	flags = kDNSServiceFlagsShareConnection;
	if( inIncludeAWDL ) flags |= kDNSServiceFlagsIncludeAWDL;
	
	newBrowse->startTicks = UpTicks();
	
	sdRef = inContext->mainRef;
	err = DNSServiceBrowse( &sdRef, flags, newBrowse->ifIndex, type->name, inDomain->name, BrowseAllBrowseCallback,
		newBrowse );
	require_noerr( err, exit );
	
	newBrowse->browse = sdRef;
	*browsePtr = newBrowse;
	newBrowse = NULL;
	
	if( newType )
	{
		*typePtr = newType;
		newType = NULL;
	}
	
exit:
	if( newBrowse )	BrowseOpFree( newBrowse );
	if( newType )	BrowseTypeFree( newType );
	return( err );
}

//===========================================================================================================================
//	BrowseAllRemoveServiceType
//===========================================================================================================================

static OSStatus
	BrowseAllRemoveServiceType(
		BrowseAllContext *	inContext,
		BrowseDomain *		inDomain,
		const char *		inName,
		uint32_t			inIfIndex )
{
	OSStatus			err;
	BrowseType *		type;
	BrowseType **		typePtr;
	BrowseOp *			browse;
	BrowseOp **			browsePtr;
	
	Unused( inContext );
	
	for( typePtr = &inDomain->typeList; ( type = *typePtr ) != NULL; typePtr = &type->next )
	{
		if( strcasecmp( type->name, inName ) == 0 ) break;
	}
	require_action_quiet( type, exit, err = kNotFoundErr );
	
	for( browsePtr = &type->browseList; ( browse = *browsePtr ) != NULL; browsePtr = &browse->next )
	{
		if( browse->ifIndex == inIfIndex ) break;
	}
	require_action_quiet( browse, exit, err = kNotFoundErr );
	
	*browsePtr = browse->next;
	BrowseOpFree( browse );
	if( !type->browseList )
	{
		*typePtr = type->next;
		BrowseTypeFree( type );
	}
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	BrowseAllAddServiceInstance
//===========================================================================================================================

static OSStatus
	BrowseAllAddServiceInstance(
		BrowseAllContext *	inContext,
		BrowseOp *			inBrowse,
		const char *		inName,
		const char *		inRegType,
		const char *		inDomain,
		uint32_t			inIfIndex )
{
	OSStatus				err;
	DNSServiceRef			sdRef;
	BrowseInstance *		instance;
	BrowseInstance **		p;
	const uint64_t			nowTicks	= UpTicks();
	BrowseInstance *		newInstance	= NULL;
	
	for( p = &inBrowse->instanceList; ( instance = *p ) != NULL; p = &instance->next )
	{
		if( ( instance->ifIndex == inIfIndex ) && ( strcasecmp( instance->name, inName ) == 0 ) ) break;
	}
	require_action_quiet( !instance, exit, err = kDuplicateErr );
	
	newInstance = (BrowseInstance *) calloc( 1, sizeof( *newInstance ) );
	require_action( newInstance, exit, err = kNoMemoryErr );
	
	++inContext->refCount;
	newInstance->context	= inContext;
	newInstance->foundTicks	= nowTicks;
	newInstance->ifIndex	= inIfIndex;
	newInstance->isTCP		= inBrowse->isTCP;
	
	newInstance->name = strdup( inName );
	require_action( newInstance->name, exit, err = kNoMemoryErr );
	
	sdRef = inContext->mainRef;
	newInstance->resolveStartTicks = UpTicks();
	err = DNSServiceResolve( &sdRef, kDNSServiceFlagsShareConnection, newInstance->ifIndex, inName, inRegType, inDomain,
		BrowseAllResolveCallback, newInstance );
	require_noerr( err, exit );
	
	newInstance->resolve = sdRef;
	*p = newInstance;
	newInstance = NULL;
	
exit:
	if( newInstance ) BrowseInstanceFree( newInstance );
	return( err );
}

//===========================================================================================================================
//	BrowseAllRemoveServiceInstance
//===========================================================================================================================

static OSStatus
	BrowseAllRemoveServiceInstance(
		BrowseAllContext *	inContext,
		BrowseOp *			inBrowse,
		const char *		inName,
		uint32_t			inIfIndex )
{
	OSStatus				err;
	BrowseInstance *		instance;
	BrowseInstance **		p;
	
	Unused( inContext );
	
	for( p = &inBrowse->instanceList; ( instance = *p ) != NULL; p = &instance->next )
	{
		if( ( instance->ifIndex == inIfIndex ) && ( strcasecmp( instance->name, inName ) == 0 ) ) break;
	}
	require_action_quiet( instance, exit, err = kNotFoundErr );
	
	*p = instance->next;
	BrowseInstanceFree( instance );
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	BrowseAllAddIPAddress
//===========================================================================================================================

#define kDiscardProtocolPort	9

static OSStatus
	BrowseAllAddIPAddress(
		BrowseAllContext *		inContext,
		BrowseInstance *		inInstance,
		const struct sockaddr *	inSockAddr )
{
	OSStatus			err;
	BrowseIPAddr *		addr;
	BrowseIPAddr **		p;
	const uint64_t		nowTicks	= UpTicks();
	BrowseIPAddr *		newAddr		= NULL;
	
	if( ( inSockAddr->sa_family != AF_INET ) && ( inSockAddr->sa_family != AF_INET6 ) )
	{
		dlogassert( "Unexpected address family: %d", inSockAddr->sa_family );
		err = kTypeErr;
		goto exit;
	}
	
	for( p = &inInstance->addrList; ( addr = *p ) != NULL; p = &addr->next )
	{
		if( SockAddrCompareAddr( &addr->sip, inSockAddr ) == 0 ) break;
	}
	require_action_quiet( !addr, exit, err = kDuplicateErr );
	
	newAddr = (BrowseIPAddr *) calloc( 1, sizeof( *newAddr ) );
	require_action( newAddr, exit, err = kNoMemoryErr );
	
	++inContext->refCount;
	newAddr->refCount	= 1;
	newAddr->context	= inContext;
	newAddr->foundTicks	= nowTicks;
	SockAddrCopy( inSockAddr, &newAddr->sip.sa );
	
	if( inInstance->isTCP && ( inInstance->port != kDiscardProtocolPort ) )
	{
		char		destination[ kSockAddrStringMaxSize ];
		
		err = SockAddrToString( &newAddr->sip, kSockAddrStringFlagsNoPort, destination );
		require_noerr( err, exit );
		
		err = AsyncConnection_Connect( &newAddr->connection, destination, -inInstance->port, kAsyncConnectionFlag_P2P,
			kAsyncConnectionNoTimeout, kSocketBufferSize_DontSet, kSocketBufferSize_DontSet,
			BrowseAllConnectionProgress, newAddr, BrowseAllConnectionHandler, newAddr, dispatch_get_main_queue() );
		require_noerr( err, exit );
		
		++newAddr->refCount;
		newAddr->connectStatus = kConnectStatus_Pending;
		++inContext->pendingConnectCount;
	}
	
	*p = newAddr;
	newAddr = NULL;
	err = kNoErr;
	
exit:
	if( newAddr ) BrowseIPAddrRelease( newAddr );
	return( err );
}

//===========================================================================================================================
//	BrowseAllRemoveIPAddress
//===========================================================================================================================

static OSStatus
	BrowseAllRemoveIPAddress(
		BrowseAllContext *		inContext,
		BrowseInstance *		inInstance,
		const struct sockaddr *	inSockAddr )
{
	OSStatus			err;
	BrowseIPAddr *		addr;
	BrowseIPAddr **		p;
	
	Unused( inContext );
	
	for( p = &inInstance->addrList; ( addr = *p ) != NULL; p = &addr->next )
	{
		if( SockAddrCompareAddr( &addr->sip.sa, inSockAddr ) == 0 ) break;
	}
	require_action_quiet( addr, exit, err = kNotFoundErr );
	
	*p = addr->next;
	BrowseIPAddrRelease( addr );
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	BrowseDomainFree
//===========================================================================================================================

static void	BrowseDomainFree( BrowseDomain *inDomain )
{
	BrowseType *		type;
	
	ForgetBrowseAllContext( &inDomain->context );
	ForgetMem( &inDomain->name );
	DNSServiceForget( &inDomain->servicesQuery );
	while( ( type = inDomain->typeList ) != NULL )
	{
		inDomain->typeList = type->next;
		BrowseTypeFree( type );
	}
	free( inDomain );
}

//===========================================================================================================================
//	BrowseTypeFree
//===========================================================================================================================

static void	BrowseTypeFree( BrowseType *inType )
{
	BrowseOp *		browse;
	
	ForgetMem( &inType->name );
	while( ( browse = inType->browseList ) != NULL )
	{
		inType->browseList = browse->next;
		BrowseOpFree( browse );
	}
	free( inType );
}

//===========================================================================================================================
//	BrowseOpFree
//===========================================================================================================================

static void	BrowseOpFree( BrowseOp *inBrowse )
{
	BrowseInstance *		instance;
	
	ForgetBrowseAllContext( &inBrowse->context );
	DNSServiceForget( &inBrowse->browse );
	while( ( instance = inBrowse->instanceList ) != NULL )
	{
		inBrowse->instanceList = instance->next;
		BrowseInstanceFree( instance );
	}
	free( inBrowse );
}

//===========================================================================================================================
//	BrowseInstanceFree
//===========================================================================================================================

static void	BrowseInstanceFree( BrowseInstance *inInstance )
{
	ForgetBrowseAllContext( &inInstance->context );
	ForgetMem( &inInstance->name );
	DNSServiceForget( &inInstance->resolve );
	DNSServiceForget( &inInstance->getAddr );
	ForgetMem( &inInstance->txtPtr );
	ForgetMem( &inInstance->hostname );
	ForgetIPAddressList( &inInstance->addrList );
	free( inInstance );
}

//===========================================================================================================================
//	BrowseIPAddrRelease
//===========================================================================================================================

static void BrowseIPAddrRelease( BrowseIPAddr *inAddr )
{
	AsyncConnection_Forget( &inAddr->connection );
	if( --inAddr->refCount == 0 )
	{
		ForgetBrowseAllContext( &inAddr->context );
		free( inAddr );
	}
}

//===========================================================================================================================
//	BrowseIPAddrReleaseList
//===========================================================================================================================

static void BrowseIPAddrReleaseList( BrowseIPAddr *inList )
{
	BrowseIPAddr *		addr;
	
	while( ( addr = inList ) != NULL )
	{
		inList = addr->next;
		BrowseIPAddrRelease( addr );
	}
}

//===========================================================================================================================
//	GetAddrInfoStressCmd
//===========================================================================================================================

typedef struct
{
	DNSServiceRef			mainRef;
	DNSServiceRef			sdRef;
	DNSServiceFlags			flags;
	unsigned int			interfaceIndex;
	unsigned int			connectionNumber;
	unsigned int			requestCount;
	unsigned int			requestCountMax;
	unsigned int			requestCountLimit;
	unsigned int			durationMinMs;
	unsigned int			durationMaxMs;
	
}	GAIStressContext;

static void	GetAddrInfoStressEvent( void *inContext );
static void	DNSSD_API
	GetAddrInfoStressCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inHostname,
		const struct sockaddr *	inSockAddr,
		uint32_t				inTTL,
		void *					inContext );

static void	GetAddrInfoStressCmd( void )
{
	OSStatus				err;
	GAIStressContext *		context = NULL;
	int						i;
	DNSServiceFlags			flags;
	uint32_t				ifIndex;
	char					ifName[ kInterfaceNameBufLen ];
	char					time[ kTimestampBufLen ];
	
	if( gGAIStress_TestDurationSecs < 0 )
	{
		FPrintF( stdout, "Invalid test duration: %d s.\n", gGAIStress_TestDurationSecs );
		err = kParamErr;
		goto exit;
	}
	if( gGAIStress_ConnectionCount <= 0 )
	{
		FPrintF( stdout, "Invalid simultaneous connection count: %d.\n", gGAIStress_ConnectionCount );
		err = kParamErr;
		goto exit;
	}
	if( gGAIStress_DurationMinMs <= 0 )
	{
		FPrintF( stdout, "Invalid minimum DNSServiceGetAddrInfo() duration: %d ms.\n", gGAIStress_DurationMinMs );
		err = kParamErr;
		goto exit;
	}
	if( gGAIStress_DurationMaxMs <= 0 )
	{
		FPrintF( stdout, "Invalid maximum DNSServiceGetAddrInfo() duration: %d ms.\n", gGAIStress_DurationMaxMs );
		err = kParamErr;
		goto exit;
	}
	if( gGAIStress_DurationMinMs > gGAIStress_DurationMaxMs )
	{
		FPrintF( stdout, "Invalid minimum and maximum DNSServiceGetAddrInfo() durations: %d ms and %d ms.\n",
			gGAIStress_DurationMinMs, gGAIStress_DurationMaxMs );
		err = kParamErr;
		goto exit;
	}
	if( gGAIStress_RequestCountMax <= 0 )
	{
		FPrintF( stdout, "Invalid maximum request count: %d.\n", gGAIStress_RequestCountMax );
		err = kParamErr;
		goto exit;
	}
	
	// Set flags.
	
	flags = GetDNSSDFlagsFromOpts();
	
	// Set interface index.
	
	err = InterfaceIndexFromArgString( gInterface, &ifIndex );
	require_noerr_quiet( err, exit );
	
	for( i = 0; i < gGAIStress_ConnectionCount; ++i )
	{
		context = (GAIStressContext *) calloc( 1, sizeof( *context ) );
		require_action( context, exit, err = kNoMemoryErr );
		
		context->flags				= flags;
		context->interfaceIndex		= ifIndex;
		context->connectionNumber	= (unsigned int)( i + 1 );
		context->requestCountMax	= (unsigned int) gGAIStress_RequestCountMax;
		context->durationMinMs		= (unsigned int) gGAIStress_DurationMinMs;
		context->durationMaxMs		= (unsigned int) gGAIStress_DurationMaxMs;
		
		dispatch_async_f( dispatch_get_main_queue(), context, GetAddrInfoStressEvent );
		context = NULL;
	}
	
	if( gGAIStress_TestDurationSecs > 0 )
	{
		dispatch_after_f( dispatch_time_seconds( gGAIStress_TestDurationSecs ), dispatch_get_main_queue(), NULL, Exit );
	}
	
	FPrintF( stdout, "Flags:                %#{flags}\n",	flags, kDNSServiceFlagsDescriptors );
	FPrintF( stdout, "Interface:            %d (%s)\n",		(int32_t) ifIndex, InterfaceIndexToName( ifIndex, ifName ) );
	FPrintF( stdout, "Test duration:        " );
	if( gGAIStress_TestDurationSecs == 0 )
	{
		FPrintF( stdout, "∞\n" );
	}
	else
	{
		FPrintF( stdout, "%d s\n", gGAIStress_TestDurationSecs );
	}
	FPrintF( stdout, "Connection count:     %d\n",		gGAIStress_ConnectionCount );
	FPrintF( stdout, "Request duration min: %d ms\n",	gGAIStress_DurationMinMs );
	FPrintF( stdout, "Request duration max: %d ms\n",	gGAIStress_DurationMaxMs );
	FPrintF( stdout, "Request count max:    %d\n",		gGAIStress_RequestCountMax );
	FPrintF( stdout, "Start time:           %s\n",		GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
	
	dispatch_main();
	
exit:
	FreeNullSafe( context );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	GetAddrInfoStressEvent
//===========================================================================================================================

#define kStressRandStrLen		5

#define kLowercaseAlphaCharSet		"abcdefghijklmnopqrstuvwxyz"

static void	GetAddrInfoStressEvent( void *inContext )
{
	GAIStressContext * const		context = (GAIStressContext *) inContext;
	OSStatus						err;
	DNSServiceRef					sdRef;
	unsigned int					nextMs;
	char							randomStr[ kStressRandStrLen + 1 ];
	char							hostname[ kStressRandStrLen + 4 + 1 ];
	char							time[ kTimestampBufLen ];
	Boolean							isConnectionNew	= false;
	static Boolean					printedHeader	= false;
	
	if( !context->mainRef || ( context->requestCount >= context->requestCountLimit ) )
	{
		DNSServiceForget( &context->mainRef );
		context->sdRef				= NULL;
		context->requestCount		= 0;
		context->requestCountLimit	= RandomRange( 1, context->requestCountMax );
		
		err = DNSServiceCreateConnection( &context->mainRef );
		require_noerr( err, exit );
		
		err = DNSServiceSetDispatchQueue( context->mainRef, dispatch_get_main_queue() );
		require_noerr( err, exit );
		
		isConnectionNew = true;
	}
	
	RandomString( kLowercaseAlphaCharSet, sizeof_string( kLowercaseAlphaCharSet ), 2, kStressRandStrLen, randomStr );
	SNPrintF( hostname, sizeof( hostname ), "%s.com", randomStr );
	
	nextMs = RandomRange( context->durationMinMs, context->durationMaxMs );
	
	if( !printedHeader )
	{
		FPrintF( stdout, "%-26s Conn  Hostname Dur (ms)\n", "Timestamp" );
		printedHeader = true;
	}
	FPrintF( stdout, "%-26s %3u%c %9s %8u\n",
		GetTimestampStr( time ), context->connectionNumber, isConnectionNew ? '*': ' ', hostname, nextMs );
	
	DNSServiceForget( &context->sdRef );
	sdRef = context->mainRef;
	err = DNSServiceGetAddrInfo( &sdRef, context->flags | kDNSServiceFlagsShareConnection, context->interfaceIndex,
		kDNSServiceProtocol_IPv4 | kDNSServiceProtocol_IPv6, hostname, GetAddrInfoStressCallback, NULL );
	require_noerr( err, exit );
	context->sdRef = sdRef;
	
	context->requestCount++;
	
	dispatch_after_f( dispatch_time_milliseconds( nextMs ), dispatch_get_main_queue(), context, GetAddrInfoStressEvent );
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	GetAddrInfoStressCallback
//===========================================================================================================================

static void DNSSD_API
	GetAddrInfoStressCallback(
		DNSServiceRef			inSDRef,
		DNSServiceFlags			inFlags,
		uint32_t				inInterfaceIndex,
		DNSServiceErrorType		inError,
		const char *			inHostname,
		const struct sockaddr *	inSockAddr,
		uint32_t				inTTL,
		void *					inContext )
{
	Unused( inSDRef );
	Unused( inFlags );
	Unused( inInterfaceIndex );
	Unused( inError );
	Unused( inHostname );
	Unused( inSockAddr );
	Unused( inTTL );
	Unused( inContext );
}

//===========================================================================================================================
//	DNSQueryCmd
//===========================================================================================================================

#define kDNSPort	53

typedef struct
{
	sockaddr_ip				serverAddr;
	uint64_t				sendTicks;
	uint8_t *				msgPtr;
	size_t					msgLen;
	size_t					msgOffset;
	const char *			name;
	dispatch_source_t		readSource;
	SocketRef				sock;
	int						timeLimitSecs;
	uint16_t				queryID;
	uint16_t				type;
	Boolean					haveTCPLen;
	Boolean					useTCP;
	Boolean					printRawRData;	// True if RDATA results are not to be formatted.
	uint8_t					msgBuf[ 512 ];
	
}	DNSQueryContext;

static void	DNSQueryPrintPrologue( const DNSQueryContext *inContext );
static void	DNSQueryReadHandler( void *inContext );
static void	DNSQueryCancelHandler( void *inContext );

static void	DNSQueryCmd( void )
{
	OSStatus				err;
	DNSQueryContext *		context = NULL;
	uint8_t *				msgPtr;
	size_t					msgLen, sendLen;
	
	// Check command parameters.
	
	if( gDNSQuery_TimeLimitSecs < -1 )
	{
		FPrintF( stdout, "Invalid wait period: %d seconds.\n", gDNSQuery_TimeLimitSecs );
		err = kParamErr;
		goto exit;
	}
	if( ( gDNSQuery_Flags < INT16_MIN ) || ( gDNSQuery_Flags > UINT16_MAX ) )
	{
		FPrintF( stdout, "DNS flags-and-codes value is out of the unsigned 16-bit range: 0x%08X.\n", gDNSQuery_Flags );
		err = kParamErr;
		goto exit;
	}
	
	// Create context.
	
	context = (DNSQueryContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	context->name			= gDNSQuery_Name;
	context->sock			= kInvalidSocketRef;
	context->timeLimitSecs	= gDNSQuery_TimeLimitSecs;
	context->queryID		= (uint16_t) Random32();
	context->useTCP			= gDNSQuery_UseTCP	 ? true : false;
	context->printRawRData	= gDNSQuery_RawRData ? true : false;
	
	err = StringToSockAddr( gDNSQuery_Server, &context->serverAddr, sizeof( context->serverAddr ), NULL );
	require_noerr( err, exit );
	if( SockAddrGetPort( &context->serverAddr ) == 0 ) SockAddrSetPort( &context->serverAddr, kDNSPort );
	
	err = RecordTypeFromArgString( gDNSQuery_Type, &context->type );
	require_noerr( err, exit );
	
	// Write query message.
	
	check_compile_time_code( sizeof( context->msgBuf ) >= ( kDNSQueryMessageMaxLen + 2 ) );
	
	msgPtr = context->useTCP ? &context->msgBuf[ 2 ] : context->msgBuf;
	err = WriteDNSQueryMessage( msgPtr, context->queryID, (uint16_t) gDNSQuery_Flags, context->name, context->type,
		kDNSServiceClass_IN, &msgLen );
	require_noerr( err, exit );
	check( msgLen <= UINT16_MAX );
	
	if( context->useTCP )
	{
		WriteBig16( context->msgBuf, msgLen );
		sendLen = 2 + msgLen;
	}
	else
	{
		sendLen = msgLen;
	}
	
	DNSQueryPrintPrologue( context );
	
	if( gDNSQuery_Verbose )
	{
		FPrintF( stdout, "DNS message to send:\n\n" );
		PrintUDNSMessage( msgPtr, msgLen, false );
		FPrintF( stdout, "---\n" );
	}
	
	if( context->useTCP )
	{
		// Create TCP socket.
		
		context->sock = socket( context->serverAddr.sa.sa_family, SOCK_STREAM, IPPROTO_TCP );
		err = map_socket_creation_errno( context->sock );
		require_noerr( err, exit );
		
		err = SocketConnect( context->sock, &context->serverAddr, 5 );
		require_noerr( err, exit );
	}
	else
	{
		// Create UDP socket.
		
		err = UDPClientSocketOpen( AF_UNSPEC, &context->serverAddr, 0, -1, NULL, &context->sock );
		require_noerr( err, exit );
	}
	
	context->sendTicks = UpTicks();
	err = SocketWriteAll( context->sock, context->msgBuf, sendLen, 5 );
	require_noerr( err, exit );
	
	if( context->timeLimitSecs == 0 ) goto exit;
	
	err = DispatchReadSourceCreate( context->sock, DNSQueryReadHandler, DNSQueryCancelHandler, context,
		&context->readSource );
	require_noerr( err, exit );
	dispatch_resume( context->readSource );
	
	if( context->timeLimitSecs > 0 )
	{
		dispatch_after_f( dispatch_time_seconds( context->timeLimitSecs ), dispatch_get_main_queue(), kExitReason_Timeout,
			Exit );
	}
	dispatch_main();
	
exit:
	if( context )
	{
		dispatch_source_forget( &context->readSource );
		ForgetSocket( &context->sock );
		free( context );
	}
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	DNSQueryPrintPrologue
//===========================================================================================================================

static void	DNSQueryPrintPrologue( const DNSQueryContext *inContext )
{
	const int		timeLimitSecs = inContext->timeLimitSecs;
	char			time[ kTimestampBufLen ];
	
	FPrintF( stdout, "Name:        %s\n",		inContext->name );
	FPrintF( stdout, "Type:        %s (%u)\n",	RecordTypeToString( inContext->type ), inContext->type );
	FPrintF( stdout, "Server:      %##a\n",		&inContext->serverAddr );
	FPrintF( stdout, "Transport:   %s\n",		inContext->useTCP ? "TCP" : "UDP" );
	FPrintF( stdout, "Time limit:  " );
	if( timeLimitSecs >= 0 )	FPrintF( stdout, "%d second%?c\n", timeLimitSecs, timeLimitSecs != 1, 's' );
	else						FPrintF( stdout, "∞\n" );
	FPrintF( stdout, "Start time:  %s\n", GetTimestampStr( time ) );
	FPrintF( stdout, "---\n" );
}

//===========================================================================================================================
//	DNSQueryReadHandler
//===========================================================================================================================

static void	DNSQueryReadHandler( void *inContext )
{
	OSStatus					err;
	const uint64_t				nowTicks	= UpTicks();
	DNSQueryContext * const		context		= (DNSQueryContext *) inContext;
	char						time[ kTimestampBufLen ];
	
	GetTimestampStr( time );
	
	if( context->useTCP )
	{
		if( !context->haveTCPLen )
		{
			err = SocketReadData( context->sock, &context->msgBuf, 2, &context->msgOffset );
			if( err == EWOULDBLOCK ) { err = kNoErr; goto exit; }
			require_noerr( err, exit );
			
			context->msgOffset	= 0;
			context->msgLen		= ReadBig16( context->msgBuf );
			context->haveTCPLen	= true;
			if( context->msgLen <= sizeof( context->msgBuf ) )
			{
				context->msgPtr = context->msgBuf;
			}
			else
			{
				context->msgPtr = (uint8_t *) malloc( context->msgLen );
				require_action( context->msgPtr, exit, err = kNoMemoryErr );
			}
		}
		
		err = SocketReadData( context->sock, context->msgPtr, context->msgLen, &context->msgOffset );
		if( err == EWOULDBLOCK ) { err = kNoErr; goto exit; }
		require_noerr( err, exit );
		context->msgOffset	= 0;
		context->haveTCPLen	= false;
	}
	else
	{
		sockaddr_ip		fromAddr;
		
		context->msgPtr = context->msgBuf;
		err = SocketRecvFrom( context->sock, context->msgPtr, sizeof( context->msgBuf ), &context->msgLen, &fromAddr,
			sizeof( fromAddr ), NULL, NULL, NULL, NULL );
		require_noerr( err, exit );
		
		check( SockAddrCompareAddr( &fromAddr, &context->serverAddr ) == 0 );
	}
	
	FPrintF( stdout, "Receive time: %s\n",			time );
	FPrintF( stdout, "Source:       %##a\n",		&context->serverAddr );
	FPrintF( stdout, "Message size: %zu\n",			context->msgLen );
	FPrintF( stdout, "RTT:          %llu ms\n\n",	UpTicksToMilliseconds( nowTicks - context->sendTicks ) );
	PrintUDNSMessage( context->msgPtr, context->msgLen, context->printRawRData );
	
	if( ( context->msgLen >= kDNSHeaderLength ) && ( DNSHeaderGetID( (DNSHeader *) context->msgPtr ) == context->queryID ) )
	{
		Exit( kExitReason_ReceivedResponse );
	}
	
exit:
	if( err ) dispatch_source_forget( &context->readSource );
}

//===========================================================================================================================
//	DNSQueryCancelHandler
//===========================================================================================================================

static void	DNSQueryCancelHandler( void *inContext )
{
	DNSQueryContext * const		context = (DNSQueryContext *) inContext;
	
	check( !context->readSource );
	ForgetSocket( &context->sock );
	if( context->msgPtr != context->msgBuf ) ForgetMem( &context->msgPtr );
	free( context );
	dispatch_async_f( dispatch_get_main_queue(), NULL, Exit );
}

#if( DNSSDUTIL_INCLUDE_DNSCRYPT )
//===========================================================================================================================
//	DNSCryptCmd
//===========================================================================================================================

#define kDNSCryptPort		443

#define kDNSCryptMinPadLength				8
#define kDNSCryptMaxPadLength				256
#define kDNSCryptBlockSize					64
#define kDNSCryptCertMinimumLength			124
#define kDNSCryptClientMagicLength			8
#define kDNSCryptResolverMagicLength		8
#define kDNSCryptHalfNonceLength			12
#define kDNSCryptCertMagicLength			4

check_compile_time( ( kDNSCryptHalfNonceLength * 2 ) == crypto_box_NONCEBYTES );

static const uint8_t		kDNSCryptCertMagic[ kDNSCryptCertMagicLength ] = { 'D', 'N', 'S', 'C' };
static const uint8_t		kDNSCryptResolverMagic[ kDNSCryptResolverMagicLength ] =
{
	0x72, 0x36, 0x66, 0x6e, 0x76, 0x57, 0x6a, 0x38
};

typedef struct
{
	uint8_t		certMagic[ kDNSCryptCertMagicLength ];
	uint8_t		esVersion[ 2 ];
	uint8_t		minorVersion[ 2 ];
	uint8_t		signature[ crypto_sign_BYTES ];
	uint8_t		publicKey[ crypto_box_PUBLICKEYBYTES ];
	uint8_t		clientMagic[ kDNSCryptClientMagicLength ];
	uint8_t		serial[ 4 ];
	uint8_t		startTime[ 4 ];
	uint8_t		endTime[ 4 ];
	uint8_t		extensions[ 1 ];	// Variably-sized extension data.
	
}	DNSCryptCert;

check_compile_time( offsetof( DNSCryptCert, extensions ) == kDNSCryptCertMinimumLength );

typedef struct
{
	uint8_t		clientMagic[ kDNSCryptClientMagicLength ];
	uint8_t		clientPublicKey[ crypto_box_PUBLICKEYBYTES ];
	uint8_t		clientNonce[ kDNSCryptHalfNonceLength ];
	uint8_t		poly1305MAC[ 16 ];
	
}	DNSCryptQueryHeader;

check_compile_time( sizeof( DNSCryptQueryHeader ) == 68 );
check_compile_time( sizeof( DNSCryptQueryHeader ) >= crypto_box_ZEROBYTES );
check_compile_time( ( sizeof( DNSCryptQueryHeader ) - crypto_box_ZEROBYTES + crypto_box_BOXZEROBYTES ) ==
	offsetof( DNSCryptQueryHeader, poly1305MAC ) );

typedef struct
{
	uint8_t		resolverMagic[ kDNSCryptResolverMagicLength ];
	uint8_t		clientNonce[ kDNSCryptHalfNonceLength ];
	uint8_t		resolverNonce[ kDNSCryptHalfNonceLength ];
	uint8_t		poly1305MAC[ 16 ];
	
}	DNSCryptResponseHeader;

check_compile_time( sizeof( DNSCryptResponseHeader ) == 48 );
check_compile_time( offsetof( DNSCryptResponseHeader, poly1305MAC ) >= crypto_box_BOXZEROBYTES );
check_compile_time( ( offsetof( DNSCryptResponseHeader, poly1305MAC ) - crypto_box_BOXZEROBYTES + crypto_box_ZEROBYTES ) ==
	sizeof( DNSCryptResponseHeader ) );

typedef struct
{
	sockaddr_ip				serverAddr;
	uint64_t				sendTicks;
	const char *			providerName;
	const char *			qname;
	const uint8_t *			certPtr;
	size_t					certLen;
	dispatch_source_t		readSource;
	size_t					msgLen;
	int						timeLimitSecs;
	uint16_t				queryID;
	uint16_t				qtype;
	Boolean					printRawRData;
	uint8_t					serverPublicSignKey[ crypto_sign_PUBLICKEYBYTES ];
	uint8_t					serverPublicKey[ crypto_box_PUBLICKEYBYTES ];
	uint8_t					clientPublicKey[ crypto_box_PUBLICKEYBYTES ];
	uint8_t					clientSecretKey[ crypto_box_SECRETKEYBYTES ];
	uint8_t					clientMagic[ kDNSCryptClientMagicLength ];
	uint8_t					clientNonce[ kDNSCryptHalfNonceLength ];
	uint8_t					nmKey[ crypto_box_BEFORENMBYTES ];
	uint8_t					msgBuf[ 512 ];
	
}	DNSCryptContext;

static void		DNSCryptReceiveCertHandler( void *inContext );
static void		DNSCryptReceiveResponseHandler( void *inContext );
static void		DNSCryptProceed( void *inContext );
static OSStatus	DNSCryptProcessCert( DNSCryptContext *inContext );
static OSStatus	DNSCryptBuildQuery( DNSCryptContext *inContext );
static OSStatus	DNSCryptSendQuery( DNSCryptContext *inContext );
static void		DNSCryptPrintCertificate( const DNSCryptCert *inCert, size_t inLen );

static void	DNSCryptCmd( void )
{
	OSStatus				err;
	DNSCryptContext *		context		= NULL;
	size_t					writtenBytes;
	size_t					totalBytes;
	SocketContext *			sockContext;
	SocketRef				sock		= kInvalidSocketRef;
	const char *			ptr;
	
	// Check command parameters.
	
	if( gDNSQuery_TimeLimitSecs < -1 )
	{
		FPrintF( stdout, "Invalid wait period: %d seconds.\n", gDNSQuery_TimeLimitSecs );
		err = kParamErr;
		goto exit;
	}
	
	// Create context.
	
	context = (DNSCryptContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	context->providerName	= gDNSCrypt_ProviderName;
	context->qname			= gDNSCrypt_Name;
	context->timeLimitSecs	= gDNSCrypt_TimeLimitSecs;
	context->printRawRData	= gDNSCrypt_RawRData ? true : false;
	
	err = crypto_box_keypair( context->clientPublicKey, context->clientSecretKey );
	require_noerr( err, exit );
	
	err = HexToData( gDNSCrypt_ProviderKey, kSizeCString, kHexToData_DefaultFlags,
		context->serverPublicSignKey, sizeof( context->serverPublicSignKey ), &writtenBytes, &totalBytes, &ptr );
	if( err || ( *ptr != '\0' ) )
	{
		FPrintF( stderr, "Failed to parse public signing key hex string (%s).\n", gDNSCrypt_ProviderKey );
		goto exit;
	}
	else if( totalBytes != sizeof( context->serverPublicSignKey ) )
	{
		FPrintF( stderr, "Public signing key contains incorrect number of hex bytes (%zu != %zu)\n",
			totalBytes, sizeof( context->serverPublicSignKey ) );
		err = kSizeErr;
		goto exit;
	}
	check( writtenBytes == totalBytes );
	
	err = StringToSockAddr( gDNSCrypt_Server, &context->serverAddr, sizeof( context->serverAddr ), NULL );
	require_noerr( err, exit );
	if( SockAddrGetPort( &context->serverAddr ) == 0 ) SockAddrSetPort( &context->serverAddr, kDNSCryptPort );
	
	err = RecordTypeFromArgString( gDNSCrypt_Type, &context->qtype );
	require_noerr( err, exit );
	
	// Write query message.
	
	context->queryID = (uint16_t) Random32();
	err = WriteDNSQueryMessage( context->msgBuf, context->queryID, kDNSHeaderFlag_RecursionDesired, context->providerName,
		kDNSServiceType_TXT, kDNSServiceClass_IN, &context->msgLen );
	require_noerr( err, exit );
	
	// Create UDP socket.
	
	err = UDPClientSocketOpen( AF_UNSPEC, &context->serverAddr, 0, -1, NULL, &sock );
	require_noerr( err, exit );
	
	// Send DNS query.
	
	context->sendTicks = UpTicks();
	err = SocketWriteAll( sock, context->msgBuf, context->msgLen, 5 );
	require_noerr( err, exit );
	
	sockContext = (SocketContext *) calloc( 1, sizeof( *sockContext ) );
	require_action( sockContext, exit, err = kNoMemoryErr );
	
	err = DispatchReadSourceCreate( sock, DNSCryptReceiveCertHandler, SocketContextCancelHandler, sockContext,
		&context->readSource );
	if( err ) ForgetMem( &sockContext );
	require_noerr( err, exit );
	
	sockContext->context	= context;
	sockContext->sock		= sock;
	sock = kInvalidSocketRef;
	dispatch_resume( context->readSource );
	
	if( context->timeLimitSecs > 0 )
	{
		dispatch_after_f( dispatch_time_seconds( context->timeLimitSecs ), dispatch_get_main_queue(), kExitReason_Timeout,
			Exit );
	}
	dispatch_main();
	
exit:
	if( context ) free( context );
	ForgetSocket( &sock );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	DNSCryptReceiveCertHandler
//===========================================================================================================================

static void	DNSCryptReceiveCertHandler( void *inContext )
{
	OSStatus					err;
	const uint64_t				nowTicks	= UpTicks();
	SocketContext * const		sockContext	= (SocketContext *) inContext;
	DNSCryptContext * const		context		= (DNSCryptContext *) sockContext->context;
	const DNSHeader *			hdr;
	sockaddr_ip					fromAddr;
	const uint8_t *				ptr;
	const uint8_t *				txtPtr;
	size_t						txtLen;
	unsigned int				answerCount, i;
	uint8_t						targetName[ kDomainNameLengthMax ];
	char						time[ kTimestampBufLen ];
	
	GetTimestampStr( time );
	
	dispatch_source_forget( &context->readSource );
	
	err = SocketRecvFrom( sockContext->sock, context->msgBuf, sizeof( context->msgBuf ), &context->msgLen,
		&fromAddr, sizeof( fromAddr ), NULL, NULL, NULL, NULL );
	require_noerr( err, exit );
	check( SockAddrCompareAddr( &fromAddr, &context->serverAddr ) == 0 );
	
	FPrintF( stdout, "Receive time: %s\n",			time );
	FPrintF( stdout, "Source:       %##a\n",		&context->serverAddr );
	FPrintF( stdout, "Message size: %zu\n",			context->msgLen );
	FPrintF( stdout, "RTT:          %llu ms\n\n",	UpTicksToMilliseconds( nowTicks - context->sendTicks ) );
	
	PrintUDNSMessage( context->msgBuf, context->msgLen, context->printRawRData );
	
	require_action_quiet( context->msgLen >= kDNSHeaderLength, exit, err = kSizeErr );
	
	hdr = (DNSHeader *) context->msgBuf;
	require_action_quiet( DNSHeaderGetID( hdr ) == context->queryID, exit, err = kMismatchErr );
	
	err = DNSMessageGetAnswerSection( context->msgBuf, context->msgLen, &ptr );
	require_noerr( err, exit );
	
	targetName[ 0 ] = 0;
	err = DomainNameAppendString( targetName, context->providerName, NULL );
	require_noerr( err, exit );
	
	answerCount = DNSHeaderGetAnswerCount( hdr );
	for( i = 0; i < answerCount; ++i )
	{
		uint16_t		type;
		uint16_t		class;
		uint8_t			name[ kDomainNameLengthMax ];
		
		err = DNSMessageExtractRecord( context->msgBuf, context->msgLen, ptr, name, &type, &class, NULL, &txtPtr, &txtLen,
			&ptr );
		require_noerr( err, exit );
		
		if( ( type == kDNSServiceType_TXT ) && ( class == kDNSServiceClass_IN ) && DomainNameEqual( name, targetName ) )
		{
			break;
		}
	}
	
	if( txtLen < ( 1 + kDNSCryptCertMinimumLength ) )
	{
		FPrintF( stderr, "TXT record length is too short (%u < %u)\n", txtLen, kDNSCryptCertMinimumLength + 1 );
		err = kSizeErr;
		goto exit;
	}
	if( txtPtr[ 0 ] < kDNSCryptCertMinimumLength )
	{
		FPrintF( stderr, "TXT record value length is too short (%u < %u)\n", txtPtr[ 0 ], kDNSCryptCertMinimumLength );
		err = kSizeErr;
		goto exit;
	}
	
	context->certLen = txtPtr[ 0 ];
	context->certPtr = &txtPtr[ 1 ];
	
	dispatch_async_f( dispatch_get_main_queue(), context, DNSCryptProceed );
	
exit:
	if( err ) Exit( NULL );
}

//===========================================================================================================================
//	DNSCryptReceiveResponseHandler
//===========================================================================================================================

static void	DNSCryptReceiveResponseHandler( void *inContext )
{
	OSStatus						err;
	const uint64_t					nowTicks	= UpTicks();
	SocketContext * const			sockContext	= (SocketContext *) inContext;
	DNSCryptContext * const			context		= (DNSCryptContext *) sockContext->context;
	sockaddr_ip						fromAddr;
	DNSCryptResponseHeader *		hdr;
	const uint8_t *					end;
	uint8_t *						ciphertext;
	uint8_t *						plaintext;
	const uint8_t *					response;
	char							time[ kTimestampBufLen ];
	uint8_t							nonce[ crypto_box_NONCEBYTES ];
	
	GetTimestampStr( time );
	
	dispatch_source_forget( &context->readSource );
	
	err = SocketRecvFrom( sockContext->sock, context->msgBuf, sizeof( context->msgBuf ), &context->msgLen,
		&fromAddr, sizeof( fromAddr ), NULL, NULL, NULL, NULL );
	require_noerr( err, exit );
	check( SockAddrCompareAddr( &fromAddr, &context->serverAddr ) == 0 );
	
	FPrintF( stdout, "Receive time: %s\n",			time );
	FPrintF( stdout, "Source:       %##a\n",		&context->serverAddr );
	FPrintF( stdout, "Message size: %zu\n",			context->msgLen );
	FPrintF( stdout, "RTT:          %llu ms\n\n",	UpTicksToMilliseconds( nowTicks - context->sendTicks ) );
	
	if( context->msgLen < sizeof( DNSCryptResponseHeader ) )
	{
		FPrintF( stderr, "DNSCrypt response is too short.\n" );
		err = kSizeErr;
		goto exit;
	}
	
	hdr = (DNSCryptResponseHeader *) context->msgBuf;
	
	if( memcmp( hdr->resolverMagic, kDNSCryptResolverMagic, kDNSCryptResolverMagicLength ) != 0 )
	{
		FPrintF( stderr, "DNSCrypt response resolver magic %#H != %#H\n",
			hdr->resolverMagic,		kDNSCryptResolverMagicLength, INT_MAX,
			kDNSCryptResolverMagic, kDNSCryptResolverMagicLength, INT_MAX );
		err = kValueErr;
		goto exit;
	}
	
	if( memcmp( hdr->clientNonce, context->clientNonce, kDNSCryptHalfNonceLength ) != 0 )
	{
		FPrintF( stderr, "DNSCrypt response client nonce mismatch.\n" );
		err = kValueErr;
		goto exit;
	}
	
	memcpy( nonce, hdr->clientNonce, crypto_box_NONCEBYTES );
	
	ciphertext = hdr->poly1305MAC - crypto_box_BOXZEROBYTES;
	memset( ciphertext, 0, crypto_box_BOXZEROBYTES );
	
	plaintext = (uint8_t *)( hdr + 1 ) - crypto_box_ZEROBYTES;
	check( plaintext == ciphertext );
	
	end = context->msgBuf + context->msgLen;
	
	err = crypto_box_open_afternm( plaintext, ciphertext, (size_t)( end - ciphertext ), nonce, context->nmKey );
	require_noerr( err, exit );
	
	response = plaintext + crypto_box_ZEROBYTES;
	PrintUDNSMessage( response, (size_t)( end - response ), context->printRawRData );
	Exit( kExitReason_ReceivedResponse );
	
exit:
	if( err ) Exit( NULL );
}

//===========================================================================================================================
//	DNSCryptProceed
//===========================================================================================================================

static void	DNSCryptProceed( void *inContext )
{
	OSStatus					err;
	DNSCryptContext * const		context = (DNSCryptContext *) inContext;
	
	err = DNSCryptProcessCert( context );
	require_noerr_quiet( err, exit );
	
	err = DNSCryptBuildQuery( context );
	require_noerr_quiet( err, exit );
	
	err = DNSCryptSendQuery( context );
	require_noerr_quiet( err, exit );
	
exit:
	if( err ) Exit( NULL );
}

//===========================================================================================================================
//	DNSCryptProcessCert
//===========================================================================================================================

static OSStatus	DNSCryptProcessCert( DNSCryptContext *inContext )
{
	OSStatus						err;
	const DNSCryptCert * const		cert	= (DNSCryptCert *) inContext->certPtr;
	const uint8_t * const			certEnd	= inContext->certPtr + inContext->certLen;
	struct timeval					now;
	time_t							startTimeSecs, endTimeSecs;
	size_t							signedLen;
	uint8_t *						tempBuf;
	unsigned long long				tempLen;
	
	DNSCryptPrintCertificate( cert, inContext->certLen );
	
	if( memcmp( cert->certMagic, kDNSCryptCertMagic, kDNSCryptCertMagicLength ) != 0 )
	{
		FPrintF( stderr, "DNSCrypt certificate magic %#H != %#H\n",
			cert->certMagic,	kDNSCryptCertMagicLength, INT_MAX,
			kDNSCryptCertMagic, kDNSCryptCertMagicLength, INT_MAX );
		err = kValueErr;
		goto exit;
	}
	
	startTimeSecs	= (time_t) ReadBig32( cert->startTime );
	endTimeSecs		= (time_t) ReadBig32( cert->endTime );
	
	gettimeofday( &now, NULL );
	if( now.tv_sec < startTimeSecs )
	{
		FPrintF( stderr, "DNSCrypt certificate start time is in the future.\n" );
		err = kDateErr;
		goto exit;
	}
	if( now.tv_sec >= endTimeSecs )
	{
		FPrintF( stderr, "DNSCrypt certificate has expired.\n" );
		err = kDateErr;
		goto exit;
	}
	
	signedLen = (size_t)( certEnd - cert->signature );
	tempBuf = (uint8_t *) malloc( signedLen );
	require_action( tempBuf, exit, err = kNoMemoryErr );
	err = crypto_sign_open( tempBuf, &tempLen, cert->signature, signedLen, inContext->serverPublicSignKey );
	free( tempBuf );
	if( err )
	{
		FPrintF( stderr, "DNSCrypt certificate failed verification.\n" );
		err = kAuthenticationErr;
		goto exit;
	}
	
	memcpy( inContext->serverPublicKey,	cert->publicKey,	crypto_box_PUBLICKEYBYTES );
	memcpy( inContext->clientMagic,		cert->clientMagic,	kDNSCryptClientMagicLength );
	
	err = crypto_box_beforenm( inContext->nmKey, inContext->serverPublicKey, inContext->clientSecretKey );
	require_noerr( err, exit );
	
	inContext->certPtr	= NULL;
	inContext->certLen	= 0;
	inContext->msgLen	= 0;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DNSCryptBuildQuery
//===========================================================================================================================

static OSStatus	DNSCryptPadQuery( uint8_t *inMsgPtr, size_t inMsgLen, size_t inMaxLen, size_t *outPaddedLen );

static OSStatus	DNSCryptBuildQuery( DNSCryptContext *inContext )
{
	OSStatus						err;
	DNSCryptQueryHeader * const		hdr			= (DNSCryptQueryHeader *) inContext->msgBuf;
	uint8_t * const					queryPtr	= (uint8_t *)( hdr + 1 );
	size_t							queryLen;
	size_t							paddedQueryLen;
	const uint8_t * const			msgLimit	= inContext->msgBuf + sizeof( inContext->msgBuf );
	const uint8_t *					padLimit;
	uint8_t							nonce[ crypto_box_NONCEBYTES ];
	
	check_compile_time_code( sizeof( inContext->msgBuf ) >= ( sizeof( DNSCryptQueryHeader ) + kDNSQueryMessageMaxLen ) );
	
	inContext->queryID = (uint16_t) Random32();
	err = WriteDNSQueryMessage( queryPtr, inContext->queryID, kDNSHeaderFlag_RecursionDesired, inContext->qname,
		inContext->qtype, kDNSServiceClass_IN, &queryLen );
	require_noerr( err, exit );
	
	padLimit = &queryPtr[ queryLen + kDNSCryptMaxPadLength ];
	if( padLimit > msgLimit ) padLimit = msgLimit;
	
	err = DNSCryptPadQuery( queryPtr, queryLen, (size_t)( padLimit - queryPtr ), &paddedQueryLen );
	require_noerr( err, exit );
	
	memset( queryPtr - crypto_box_ZEROBYTES, 0, crypto_box_ZEROBYTES );
	RandomBytes( inContext->clientNonce, kDNSCryptHalfNonceLength );
	memcpy( nonce, inContext->clientNonce, kDNSCryptHalfNonceLength );
	memset( &nonce[ kDNSCryptHalfNonceLength ], 0, kDNSCryptHalfNonceLength );
	
	err = crypto_box_afternm( queryPtr - crypto_box_ZEROBYTES, queryPtr - crypto_box_ZEROBYTES,
		paddedQueryLen + crypto_box_ZEROBYTES, nonce, inContext->nmKey );
	require_noerr( err, exit );
	
	memcpy( hdr->clientMagic,		inContext->clientMagic,		kDNSCryptClientMagicLength );
	memcpy( hdr->clientPublicKey,	inContext->clientPublicKey,	crypto_box_PUBLICKEYBYTES );
	memcpy( hdr->clientNonce,		nonce,						kDNSCryptHalfNonceLength );
	
	inContext->msgLen = (size_t)( &queryPtr[ paddedQueryLen ] - inContext->msgBuf );
	
exit:
	return( err );
}

static OSStatus	DNSCryptPadQuery( uint8_t *inMsgPtr, size_t inMsgLen, size_t inMaxLen, size_t *outPaddedLen )
{
	OSStatus		err;
	size_t			paddedLen;
	
	require_action_quiet( ( inMsgLen + kDNSCryptMinPadLength ) <= inMaxLen, exit, err = kSizeErr );
	
	paddedLen = inMsgLen + kDNSCryptMinPadLength +
		arc4random_uniform( (uint32_t)( inMaxLen - ( inMsgLen + kDNSCryptMinPadLength ) + 1 ) );
	paddedLen += ( kDNSCryptBlockSize - ( paddedLen % kDNSCryptBlockSize ) );
	if( paddedLen > inMaxLen ) paddedLen = inMaxLen;
	
	inMsgPtr[ inMsgLen ] = 0x80;
	memset( &inMsgPtr[ inMsgLen + 1 ], 0, paddedLen - ( inMsgLen + 1 ) );
	
	if( outPaddedLen ) *outPaddedLen = paddedLen;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DNSCryptSendQuery
//===========================================================================================================================

static OSStatus	DNSCryptSendQuery( DNSCryptContext *inContext )
{
	OSStatus			err;
	SocketContext *		sockContext;
	SocketRef			sock = kInvalidSocketRef;
	
	check( inContext->msgLen > 0 );
	check( !inContext->readSource );
	
	err = UDPClientSocketOpen( AF_UNSPEC, &inContext->serverAddr, 0, -1, NULL, &sock );
	require_noerr( err, exit );
	
	inContext->sendTicks = UpTicks();
	err = SocketWriteAll( sock, inContext->msgBuf, inContext->msgLen, 5 );
	require_noerr( err, exit );
	
	sockContext = (SocketContext *) calloc( 1, sizeof( *sockContext ) );
	require_action( sockContext, exit, err = kNoMemoryErr );
	
	err = DispatchReadSourceCreate( sock, DNSCryptReceiveResponseHandler, SocketContextCancelHandler, sockContext,
		&inContext->readSource );
	if( err ) ForgetMem( &sockContext );
	require_noerr( err, exit );
	
	sockContext->context	= inContext;
	sockContext->sock		= sock;
	sock = kInvalidSocketRef;
	
	dispatch_resume( inContext->readSource );
	
exit:
	ForgetSocket( &sock );
	return( err );
}

//===========================================================================================================================
//	DNSCryptPrintCertificate
//===========================================================================================================================

#define kCertTimeStrBufLen		32

static char *	CertTimeStr( time_t inTime, char inBuffer[ kCertTimeStrBufLen ] );

static void	DNSCryptPrintCertificate( const DNSCryptCert *inCert, size_t inLen )
{
	time_t		startTime, endTime;
	int			extLen;
	char		timeBuf[ kCertTimeStrBufLen ];
	
	check( inLen >= kDNSCryptCertMinimumLength );
	
	startTime	= (time_t) ReadBig32( inCert->startTime );
	endTime		= (time_t) ReadBig32( inCert->endTime );
	
	FPrintF( stdout, "DNSCrypt certificate (%zu bytes):\n", inLen );
	FPrintF( stdout, "Cert Magic:    %#H\n", inCert->certMagic, kDNSCryptCertMagicLength, INT_MAX );
	FPrintF( stdout, "ES Version:    %u\n",	ReadBig16( inCert->esVersion ) );
	FPrintF( stdout, "Minor Version: %u\n",	ReadBig16( inCert->minorVersion ) );
	FPrintF( stdout, "Signature:     %H\n",	inCert->signature, crypto_sign_BYTES / 2, INT_MAX );
	FPrintF( stdout, "               %H\n",	&inCert->signature[ crypto_sign_BYTES / 2 ], crypto_sign_BYTES / 2, INT_MAX );
	FPrintF( stdout, "Public Key:    %H\n", inCert->publicKey, sizeof( inCert->publicKey ), INT_MAX );
	FPrintF( stdout, "Client Magic:  %H\n", inCert->clientMagic, kDNSCryptClientMagicLength, INT_MAX );
	FPrintF( stdout, "Serial:        %u\n",	ReadBig32( inCert->serial ) );
	FPrintF( stdout, "Start Time:    %u (%s)\n", (uint32_t) startTime, CertTimeStr( startTime, timeBuf ) );
	FPrintF( stdout, "End Time:      %u (%s)\n", (uint32_t) endTime, CertTimeStr( endTime, timeBuf ) );
	
	if( inLen > kDNSCryptCertMinimumLength )
	{
		extLen = (int)( inLen - kDNSCryptCertMinimumLength );
		FPrintF( stdout, "Extensions:    %.1H\n", inCert->extensions, extLen, extLen );
	}
	FPrintF( stdout, "\n" );
}

static char *	CertTimeStr( time_t inTime, char inBuffer[ kCertTimeStrBufLen ] )
{
	struct tm *		tm;
	
	tm = localtime( &inTime );
	if( !tm )
	{
		dlogassert( "localtime() returned a NULL pointer.\n" );
		*inBuffer = '\0';
	}
	else
	{
		strftime( inBuffer, kCertTimeStrBufLen, "%a %b %d %H:%M:%S %Z %Y", tm );
	}
	
	return( inBuffer );
}

#endif	// DNSSDUTIL_INCLUDE_DNSCRYPT

//===========================================================================================================================
//	MDNSQueryCmd
//===========================================================================================================================

#define kMDNSMulticastAddressIPv4	0xE00000FB	// 224.0.0.251
#define kMDNSPort					5353

#define kDefaultMDNSMessageID		0
#define kDefaultMDNSQueryFlags		0

typedef struct
{
	const char *			qname;
	dispatch_source_t		readSourceV4;
	dispatch_source_t		readSourceV6;
	int						receivedMsgCount;
	int						localPort;
	uint32_t				ifIndex;
	uint16_t				type;
	Boolean					isQU;
	Boolean					printRawRData;
	Boolean					useIPv4;
	Boolean					useIPv6;
	Boolean					allResponses;
	char					ifName[ IF_NAMESIZE + 1 ];
	uint8_t					domainName[ kDomainNameLengthMax ];
	uint8_t					msgBuf[ 8940 ];
	
}	MDNSQueryContext;

static void	MDNSQueryPrintPrologue( const MDNSQueryContext *inContext );
static void	MDNSQueryReadHandler( void *inContext );

static void	MDNSQueryCmd( void )
{
	OSStatus				err;
	MDNSQueryContext *		context;
	struct sockaddr_in		mcastAddr4;
	struct sockaddr_in6		mcastAddr6;
	SocketRef				sockV4 = kInvalidSocketRef;
	SocketRef				sockV6 = kInvalidSocketRef;
	ssize_t					n;
	const char *			ifNamePtr;
	SocketContext *			sockContext;
	size_t					msgLen;
	
	context = (MDNSQueryContext *) calloc( 1, sizeof( *context ) );
	require_action( context, exit, err = kNoMemoryErr );
	
	context->qname			= gMDNSQuery_Name;
	context->isQU			= gMDNSQuery_IsQU		  ? true : false;
	context->printRawRData	= gMDNSQuery_RawRData	  ? true : false;
	context->allResponses	= gMDNSQuery_AllResponses ? true : false;
	context->useIPv4		= ( gMDNSQuery_UseIPv4 || !gMDNSQuery_UseIPv6 ) ? true : false;
	context->useIPv6		= ( gMDNSQuery_UseIPv6 || !gMDNSQuery_UseIPv4 ) ? true : false;
	
	err = InterfaceIndexFromArgString( gInterface, &context->ifIndex );
	require_noerr_quiet( err, exit );
	
	ifNamePtr = if_indextoname( context->ifIndex, context->ifName );
	require_action( ifNamePtr, exit, err = kNameErr );
	
	err = RecordTypeFromArgString( gMDNSQuery_Type, &context->type );
	require_noerr( err, exit );
	
	// Set up IPv4 socket.
	
	if( context->useIPv4 )
	{
		err = ServerSocketOpen( AF_INET, SOCK_DGRAM, IPPROTO_UDP,
			gMDNSQuery_SourcePort ? gMDNSQuery_SourcePort : ( context->isQU ? context->localPort : kMDNSPort ),
			&context->localPort, kSocketBufferSize_DontSet, &sockV4 );
		require_noerr( err, exit );
		
		err = SocketSetMulticastInterface( sockV4, ifNamePtr, context->ifIndex );
		require_noerr( err, exit );
		
		err = setsockopt( sockV4, IPPROTO_IP, IP_MULTICAST_LOOP, (char *) &(uint8_t){ 1 }, (socklen_t) sizeof( uint8_t ) );
		err = map_socket_noerr_errno( sockV4, err );
		require_noerr( err, exit );
		
		memset( &mcastAddr4, 0, sizeof( mcastAddr4 ) );
		SIN_LEN_SET( &mcastAddr4 );
		mcastAddr4.sin_family		= AF_INET;
		mcastAddr4.sin_port			= htons( kMDNSPort );
		mcastAddr4.sin_addr.s_addr	= htonl( kMDNSMulticastAddressIPv4 );
		
		if( !context->isQU && ( context->localPort == kMDNSPort ) )
		{
			SocketJoinMulticast( sockV4, &mcastAddr4, ifNamePtr, context->ifIndex );
			require_noerr( err, exit );
		}
	}
	
	// Set up IPv6 socket.
	
	if( context->useIPv6 )
	{
		err = ServerSocketOpen( AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			gMDNSQuery_SourcePort ? gMDNSQuery_SourcePort : ( context->isQU ? context->localPort : kMDNSPort ),
			&context->localPort, kSocketBufferSize_DontSet, &sockV6 );
		require_noerr( err, exit );
		
		err = SocketSetMulticastInterface( sockV6, ifNamePtr, context->ifIndex );
		require_noerr( err, exit );
		
		err = setsockopt( sockV6, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *) &(int){ 1 }, (socklen_t) sizeof( int ) );
		err = map_socket_noerr_errno( sockV6, err );
		require_noerr( err, exit );
		
		memset( &mcastAddr6, 0, sizeof( mcastAddr6 ) );
		SIN6_LEN_SET( &mcastAddr6 );
		mcastAddr6.sin6_family	= AF_INET6;
		mcastAddr6.sin6_port	= htons( kMDNSPort );
		mcastAddr6.sin6_addr.s6_addr[  0 ] = 0xFF;	// mDNS IPv6 multicast address FF02::FB
		mcastAddr6.sin6_addr.s6_addr[  1 ] = 0x02;
		mcastAddr6.sin6_addr.s6_addr[ 15 ] = 0xFB;
		
		if( !context->isQU && ( context->localPort == kMDNSPort ) )
		{
			SocketJoinMulticast( sockV6, &mcastAddr6, ifNamePtr, context->ifIndex );
			require_noerr( err, exit );
		}
	}
	
	// Write mDNS query message.
	
	check_compile_time_code( sizeof( context->msgBuf ) >= kDNSQueryMessageMaxLen );
	err = WriteDNSQueryMessage( context->msgBuf, kDefaultMDNSMessageID, kDefaultMDNSQueryFlags, context->qname,
		context->type,
		context->isQU ? ( kDNSServiceClass_IN | kQClassUnicastResponseBit ) : kDNSServiceClass_IN, &msgLen );
	require_noerr( err, exit );
	
	// Print prologue.
	
	MDNSQueryPrintPrologue( context );
	
	if( context->useIPv4 )
	{
		n = sendto( sockV4, context->msgBuf, msgLen, 0, (struct sockaddr *) &mcastAddr4, (socklen_t) sizeof( mcastAddr4 ) );
		err = map_socket_value_errno( sockV4, n == (ssize_t) msgLen, n );
		require_noerr_quiet( err, exit );
		
		sockContext = (SocketContext *) calloc( 1, sizeof( *sockContext ) );
		require_action( sockContext, exit, err = kNoMemoryErr );
		
		err = DispatchReadSourceCreate( sockV4, MDNSQueryReadHandler, SocketContextCancelHandler, sockContext,
			&context->readSourceV4 );
		if( err ) ForgetMem( &sockContext );
		require_noerr( err, exit );
		
		sockContext->context	= context;
		sockContext->sock		= sockV4;
		sockV4 = kInvalidSocketRef;
		dispatch_resume( context->readSourceV4 );
	}
	
	if( context->useIPv6 )
	{
		n = sendto( sockV6, context->msgBuf, msgLen, 0, (struct sockaddr *) &mcastAddr6, (socklen_t) sizeof( mcastAddr6 ) );
		err = map_socket_value_errno( sockV6, n == (ssize_t) msgLen, n );
		require_noerr_quiet( err, exit );
		
		sockContext = (SocketContext *) calloc( 1, sizeof( *sockContext ) );
		require_action( sockContext, exit, err = kNoMemoryErr );
		
		err = DispatchReadSourceCreate( sockV6, MDNSQueryReadHandler, SocketContextCancelHandler, sockContext,
			&context->readSourceV6 );
		if( err ) ForgetMem( &sockContext );
		require_noerr( err, exit );
		
		sockContext->context	= context;
		sockContext->sock		= sockV6;
		sockV6 = kInvalidSocketRef;
		dispatch_resume( context->readSourceV6 );
	}
	
	dispatch_main();
	
exit:
	ForgetSocket( &sockV4 );
	ForgetSocket( &sockV6 );
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	MDNSQueryPrintPrologue
//===========================================================================================================================

static void	MDNSQueryPrintPrologue( const MDNSQueryContext *inContext )
{
	char		time[ kTimestampBufLen ];
	
	FPrintF( stdout, "Interface:   %d (%s)\n",	(int32_t) inContext->ifIndex, inContext->ifName );
	FPrintF( stdout, "Name:        %s\n",		inContext->qname );
	FPrintF( stdout, "Type:        %s (%u)\n",	RecordTypeToString( inContext->type ), inContext->type );
	FPrintF( stdout, "Class:       IN (%s)\n",	inContext->isQU ? "QU" : "QM" );
	FPrintF( stdout, "Local port:  %d\n",		inContext->localPort );
	FPrintF( stdout, "Start time:  %s\n",		GetTimestampStr( time ) );
}

//===========================================================================================================================
//	MDNSQueryReadHandler
//===========================================================================================================================

static void	MDNSQueryReadHandler( void *inContext )
{
	OSStatus						err;
	SocketContext * const			sockContext	= (SocketContext *) inContext;
	MDNSQueryContext * const		context		= (MDNSQueryContext *) sockContext->context;
	size_t							msgLen;
	sockaddr_ip						fromAddr;
	char							time[ kTimestampBufLen ];
	Boolean							foundAnswer	= false;
	
	GetTimestampStr( time );
	
	err = SocketRecvFrom( sockContext->sock, context->msgBuf, sizeof( context->msgBuf ), &msgLen, &fromAddr,
		sizeof( fromAddr ), NULL, NULL, NULL, NULL );
	require_noerr( err, exit );
	
	if( !context->allResponses && ( msgLen >= kDNSHeaderLength ) )
	{
		const uint8_t *				ptr;
		const DNSHeader * const		hdr = (DNSHeader *) context->msgBuf;
		unsigned int				rrCount, i;
		uint16_t					type, class;
		uint8_t						domainName[ kDomainNameLengthMax ];
		
		err = DNSMessageGetAnswerSection( context->msgBuf, msgLen, &ptr );
		require_noerr( err, exit );
		
		if( context->domainName[ 0 ] == 0 )
		{
			err = DomainNameAppendString( context->domainName, context->qname, NULL );
			require_noerr( err, exit );
		}
		
		rrCount = DNSHeaderGetAnswerCount( hdr ) + DNSHeaderGetAuthorityCount( hdr ) + DNSHeaderGetAdditionalCount( hdr );
		for( i = 0; i < rrCount; ++i )
		{
			err = DNSMessageExtractRecord( context->msgBuf, msgLen, ptr, domainName, &type, &class, NULL, NULL, NULL, &ptr );
			require_noerr( err, exit );
			
			if( ( type == context->type ) && DomainNameEqual( domainName, context->domainName ) )
			{
				foundAnswer = true;
				break;
			}
		}
	}
	if( context->allResponses || foundAnswer )
	{
		FPrintF( stdout, "---\n" );
		FPrintF( stdout, "Receive time: %s\n",		time );
		FPrintF( stdout, "Source:       %##a\n",	&fromAddr );
		FPrintF( stdout, "Message size: %zu\n\n",	msgLen );
		
		PrintMDNSMessage( context->msgBuf, msgLen, context->printRawRData );
	}
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	PIDToUUIDCmd
//===========================================================================================================================

static void	PIDToUUIDCmd( void )
{
	OSStatus							err;
	int									n;
	struct proc_uniqidentifierinfo		info;
	
	n = proc_pidinfo( gPIDToUUID_PID, PROC_PIDUNIQIDENTIFIERINFO, 1, &info, sizeof( info ) );
	require_action_quiet( n == (int) sizeof( info ), exit, err = kUnknownErr );
	
	FPrintF( stdout, "%#U\n", info.p_uuid );
	err = kNoErr;
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	DaemonVersionCmd
//===========================================================================================================================

static void	DaemonVersionCmd( void )
{
	OSStatus		err;
	uint32_t		size, version;
	char			strBuf[ 16 ];
	
	size = (uint32_t) sizeof( version );
	err = DNSServiceGetProperty( kDNSServiceProperty_DaemonVersion, &version, &size );
	require_noerr( err, exit );
	
	FPrintF( stdout, "Daemon version: %s\n", SourceVersionToCString( version, strBuf ) );
	
exit:
	if( err ) exit( 1 );
}

//===========================================================================================================================
//	Exit
//===========================================================================================================================

static void	Exit( void *inContext )
{
	const char * const		reason = (const char *) inContext;
	char					time[ kTimestampBufLen ];
	
	FPrintF( stdout, "---\n" );
	FPrintF( stdout, "End time:   %s\n", GetTimestampStr( time ) );
	if( reason ) FPrintF( stdout, "End reason: %s\n", reason );
	exit( gExitCode );
}

//===========================================================================================================================
//	GetTimestampStr
//===========================================================================================================================

static char *	GetTimestampStr( char inBuffer[ kTimestampBufLen ] )
{
	struct timeval		now;
	struct tm *			tm;
	size_t				len;
	
	gettimeofday( &now, NULL );
	tm = localtime( &now.tv_sec );
	require_action( tm, exit, *inBuffer = '\0' );
	
	len = strftime( inBuffer, kTimestampBufLen, "%Y-%m-%d %H:%M:%S", tm );
	SNPrintF( &inBuffer[ len ], kTimestampBufLen - len, ".%06u", (unsigned int) now.tv_usec );
	
exit:
	return( inBuffer );
}

//===========================================================================================================================
//	GetDNSSDFlagsFromOpts
//===========================================================================================================================

static DNSServiceFlags	GetDNSSDFlagsFromOpts( void )
{
	DNSServiceFlags		flags;
	
	flags = (DNSServiceFlags) gDNSSDFlags;
	if( flags & kDNSServiceFlagsShareConnection )
	{
		FPrintF( stderr, "*** Warning: kDNSServiceFlagsShareConnection (0x%X) is explicitly set in flag parameters.\n",
			kDNSServiceFlagsShareConnection );
	}
	
	if( gDNSSDFlag_BrowseDomains )			flags |= kDNSServiceFlagsBrowseDomains;
	if( gDNSSDFlag_DenyCellular )			flags |= kDNSServiceFlagsDenyCellular;
	if( gDNSSDFlag_DenyExpensive )			flags |= kDNSServiceFlagsDenyExpensive;
	if( gDNSSDFlag_ForceMulticast )			flags |= kDNSServiceFlagsForceMulticast;
	if( gDNSSDFlag_IncludeAWDL )			flags |= kDNSServiceFlagsIncludeAWDL;
	if( gDNSSDFlag_NoAutoRename )			flags |= kDNSServiceFlagsNoAutoRename;
	if( gDNSSDFlag_PathEvaluationDone )		flags |= kDNSServiceFlagsPathEvaluationDone;
	if( gDNSSDFlag_RegistrationDomains )	flags |= kDNSServiceFlagsRegistrationDomains;
	if( gDNSSDFlag_ReturnIntermediates )	flags |= kDNSServiceFlagsReturnIntermediates;
	if( gDNSSDFlag_Shared )					flags |= kDNSServiceFlagsShared;
	if( gDNSSDFlag_SuppressUnusable )		flags |= kDNSServiceFlagsSuppressUnusable;
	if( gDNSSDFlag_Timeout )				flags |= kDNSServiceFlagsTimeout;
	if( gDNSSDFlag_UnicastResponse )		flags |= kDNSServiceFlagsUnicastResponse;
	if( gDNSSDFlag_Unique )					flags |= kDNSServiceFlagsUnique;
	
	return( flags );
}

//===========================================================================================================================
//	CreateConnectionFromArgString
//===========================================================================================================================

static OSStatus
	CreateConnectionFromArgString(
		const char *			inString,
		dispatch_queue_t		inQueue,
		DNSServiceRef *			outSDRef,
		ConnectionDesc *		outDesc )
{
	OSStatus			err;
	DNSServiceRef		sdRef = NULL;
	ConnectionType		type;
	int32_t				pid = -1;	// Initializing because the analyzer claims pid may be used uninitialized.
	uint8_t				uuid[ 16 ];
	
	if( strcasecmp( inString, kConnectionArg_Normal ) == 0 )
	{
		err = DNSServiceCreateConnection( &sdRef );
		require_noerr( err, exit );
		type = kConnectionType_Normal;
	}
	else if( stricmp_prefix( inString, kConnectionArgPrefix_PID ) == 0 )
	{
		const char * const		pidStr = inString + sizeof_string( kConnectionArgPrefix_PID );
		
		err = StringToInt32( pidStr, &pid );
		if( err )
		{
			FPrintF( stderr, "Invalid delegate connection PID value: %s\n", pidStr );
			err = kParamErr;
			goto exit;
		}
		
		memset( uuid, 0, sizeof( uuid ) );
		err = DNSServiceCreateDelegateConnection( &sdRef, pid, uuid );
		if( err )
		{
			FPrintF( stderr, "DNSServiceCreateDelegateConnection() returned %#m for PID %d\n", err, pid );
			goto exit;
		}
		type = kConnectionType_DelegatePID;
	}
	else if( stricmp_prefix( inString, kConnectionArgPrefix_UUID ) == 0 )
	{
		const char * const		uuidStr = inString + sizeof_string( kConnectionArgPrefix_UUID );
		
		check_compile_time_code( sizeof( uuid ) == sizeof( uuid_t ) );
		
		err = StringToUUID( uuidStr, kSizeCString, false, uuid );
		if( err )
		{
			FPrintF( stderr, "Invalid delegate connection UUID value: %s\n", uuidStr );
			err = kParamErr;
			goto exit;
		}
		
		err = DNSServiceCreateDelegateConnection( &sdRef, 0, uuid );
		if( err )
		{
			FPrintF( stderr, "DNSServiceCreateDelegateConnection() returned %#m for UUID %#U\n", err, uuid );
			goto exit;
		}
		type = kConnectionType_DelegateUUID;
	}
	else
	{
		FPrintF( stderr, "Unrecognized connection string \"%s\".\n", inString );
		err = kParamErr;
		goto exit;
	}
	
	err = DNSServiceSetDispatchQueue( sdRef, inQueue );
	require_noerr( err, exit );
	
	*outSDRef = sdRef;
	if( outDesc )
	{
		outDesc->type = type;
		if(      type == kConnectionType_DelegatePID )	outDesc->delegate.pid = pid;
		else if( type == kConnectionType_DelegateUUID )	memcpy( outDesc->delegate.uuid, uuid, 16 );
	}
	sdRef = NULL;
	
exit:
	if( sdRef ) DNSServiceRefDeallocate( sdRef );
	return( err );
}

//===========================================================================================================================
//	InterfaceIndexFromArgString
//===========================================================================================================================

static OSStatus	InterfaceIndexFromArgString( const char *inString, uint32_t *outIndex )
{
	OSStatus		err;
	uint32_t		ifIndex;
	
	if( inString )
	{
		ifIndex = if_nametoindex( inString );
		if( ifIndex == 0 )
		{
			err = StringToUInt32( inString, &ifIndex );
			if( err )
			{
				FPrintF( stderr, "Invalid interface value: %s\n", inString );
				err = kParamErr;
				goto exit;
			}
		}
	}
	else
	{
		ifIndex	= 0;
	}
	
	*outIndex = ifIndex;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	RecordDataFromArgString
//===========================================================================================================================

#define kRDataMaxLen		UINT16_C( 0xFFFF )

static OSStatus	RecordDataFromArgString( const char *inString, uint8_t **outDataPtr, size_t *outDataLen )
{
	OSStatus		err;
	uint8_t *		dataPtr = NULL;
	size_t			dataLen;
	DataBuffer		dataBuf;
	
	DataBuffer_Init( &dataBuf, NULL, 0, kRDataMaxLen );
	
	if( stricmp_prefix( inString, kRDataArgPrefix_String ) == 0 )
	{
		const char * const		strPtr = inString + sizeof_string( kRDataArgPrefix_String );
		const size_t			strLen = strlen( strPtr );
		size_t					copiedLen;
		size_t					totalLen;
		
		if( strLen > 0 )
		{
			require_action( strLen <= kRDataMaxLen, exit, err = kSizeErr );
			dataPtr = (uint8_t *) malloc( strLen );
			require_action( dataPtr, exit, err = kNoMemoryErr );
			
			copiedLen = 0;
			ParseQuotedEscapedString( strPtr, strPtr + strLen, "", (char *) dataPtr, strLen, &copiedLen, &totalLen, NULL );
			check( copiedLen == totalLen );
			dataLen = copiedLen;
		}
		else
		{
			dataPtr = NULL;
			dataLen = 0;
		}
	}
	else if( stricmp_prefix( inString, kRDataArgPrefix_HexString ) == 0 )
	{
		const char * const		strPtr = inString + sizeof_string( kRDataArgPrefix_HexString );
		
		err = HexToDataCopy( strPtr, kSizeCString, kHexToData_DefaultFlags, &dataPtr, &dataLen, NULL );
		require_noerr( err, exit );
		require_action( dataLen <= kRDataMaxLen, exit, err = kSizeErr );
	}
	else if( stricmp_prefix( inString, kRDataArgPrefix_File ) == 0 )
	{
		const char * const		path = inString + sizeof_string( kRDataArgPrefix_File );
		
		err = CopyFileDataByPath( path, (char **) &dataPtr, &dataLen );
		require_noerr( err, exit );
		require_action( dataLen <= kRDataMaxLen, exit, err = kSizeErr );
	}
	else if( stricmp_prefix( inString, kRDataArgPrefix_TXT ) == 0 )
	{
		const char *			strPtr = inString + sizeof_string( kRDataArgPrefix_TXT );
		const char * const		strEnd = strPtr + strlen( strPtr );
		
		while( strPtr < strEnd )
		{
			size_t		copiedLen, totalLen;
			uint8_t		kvBuf[ 1 + 255 + 1 ];	// Length byte + max key-value length + 1 for NUL terminator.
			
			err = ParseEscapedString( strPtr, strEnd, ',', (char *) &kvBuf[ 1 ], sizeof( kvBuf ) - 1,
				&copiedLen, &totalLen, &strPtr );
			require_noerr_quiet( err, exit );
			check( copiedLen == totalLen );
			if( totalLen > 255 )
			{
				FPrintF( stderr, "TXT key-value pair length %zu is too long (> 255 bytes).\n", totalLen );
				err = kParamErr;
				goto exit;
			}
			
			kvBuf[ 0 ] = (uint8_t) copiedLen;
			err = DataBuffer_Append( &dataBuf, kvBuf, 1 + kvBuf[ 0 ] );
			require_noerr( err, exit );
		}
		
		err = DataBuffer_Commit( &dataBuf, NULL, NULL );
		require_noerr( err, exit );
		
		err = DataBuffer_Detach( &dataBuf, &dataPtr, &dataLen );
		require_noerr( err, exit );
	}
	else
	{
		FPrintF( stderr, "Unrecognized record data string \"%s\".\n", inString );
		err = kParamErr;
		goto exit;
	}
	err = kNoErr;
	
	*outDataLen = dataLen;
	*outDataPtr = dataPtr;
	dataPtr = NULL;
	
exit:
	DataBuffer_Free( &dataBuf );
	FreeNullSafe( dataPtr );
	return( err );
}

//===========================================================================================================================
//	RecordTypeFromArgString
//===========================================================================================================================

typedef struct
{
	uint16_t			value;	// Record type's numeric value.
	const char *		name;	// Record type's name as a string (e.g., "A", "PTR", "SRV").
	
}	RecordType;

static const RecordType		kRecordTypes[] =
{
	// Common types.
	
	{ kDNSServiceType_A,			"A" },
	{ kDNSServiceType_AAAA,			"AAAA" },
	{ kDNSServiceType_PTR,			"PTR" },
	{ kDNSServiceType_SRV,			"SRV" },
	{ kDNSServiceType_TXT,			"TXT" },
	{ kDNSServiceType_CNAME,		"CNAME" },
	{ kDNSServiceType_SOA,			"SOA" },
	{ kDNSServiceType_NSEC,			"NSEC" },
	{ kDNSServiceType_NS,			"NS" },
	{ kDNSServiceType_MX,			"MX" },
	{ kDNSServiceType_ANY,			"ANY" },
	{ kDNSServiceType_OPT,			"OPT" },
	
	// Less common types.
	
	{ kDNSServiceType_MD,			"MD" },
	{ kDNSServiceType_NS,			"NS" },
	{ kDNSServiceType_MD,			"MD" },
	{ kDNSServiceType_MF,			"MF" },
	{ kDNSServiceType_MB,			"MB" },
	{ kDNSServiceType_MG,			"MG" },
	{ kDNSServiceType_MR,			"MR" },
	{ kDNSServiceType_NULL,			"NULL" },
	{ kDNSServiceType_WKS,			"WKS" },
	{ kDNSServiceType_HINFO,		"HINFO" },
	{ kDNSServiceType_MINFO,		"MINFO" },
	{ kDNSServiceType_RP,			"RP" },
	{ kDNSServiceType_AFSDB,		"AFSDB" },
	{ kDNSServiceType_X25,			"X25" },
	{ kDNSServiceType_ISDN,			"ISDN" },
	{ kDNSServiceType_RT,			"RT" },
	{ kDNSServiceType_NSAP,			"NSAP" },
	{ kDNSServiceType_NSAP_PTR,		"NSAP_PTR" },
	{ kDNSServiceType_SIG,			"SIG" },
	{ kDNSServiceType_KEY,			"KEY" },
	{ kDNSServiceType_PX,			"PX" },
	{ kDNSServiceType_GPOS,			"GPOS" },
	{ kDNSServiceType_LOC,			"LOC" },
	{ kDNSServiceType_NXT,			"NXT" },
	{ kDNSServiceType_EID,			"EID" },
	{ kDNSServiceType_NIMLOC,		"NIMLOC" },
	{ kDNSServiceType_ATMA,			"ATMA" },
	{ kDNSServiceType_NAPTR,		"NAPTR" },
	{ kDNSServiceType_KX,			"KX" },
	{ kDNSServiceType_CERT,			"CERT" },
	{ kDNSServiceType_A6,			"A6" },
	{ kDNSServiceType_DNAME,		"DNAME" },
	{ kDNSServiceType_SINK,			"SINK" },
	{ kDNSServiceType_APL,			"APL" },
	{ kDNSServiceType_DS,			"DS" },
	{ kDNSServiceType_SSHFP,		"SSHFP" },
	{ kDNSServiceType_IPSECKEY,		"IPSECKEY" },
	{ kDNSServiceType_RRSIG,		"RRSIG" },
	{ kDNSServiceType_DNSKEY,		"DNSKEY" },
	{ kDNSServiceType_DHCID,		"DHCID" },
	{ kDNSServiceType_NSEC3,		"NSEC3" },
	{ kDNSServiceType_NSEC3PARAM,	"NSEC3PARAM" },
	{ kDNSServiceType_HIP,			"HIP" },
	{ kDNSServiceType_SPF,			"SPF" },
	{ kDNSServiceType_UINFO,		"UINFO" },
	{ kDNSServiceType_UID,			"UID" },
	{ kDNSServiceType_GID,			"GID" },
	{ kDNSServiceType_UNSPEC,		"UNSPEC" },
	{ kDNSServiceType_TKEY,			"TKEY" },
	{ kDNSServiceType_TSIG,			"TSIG" },
	{ kDNSServiceType_IXFR,			"IXFR" },
	{ kDNSServiceType_AXFR,			"AXFR" },
	{ kDNSServiceType_MAILB,		"MAILB" },
	{ kDNSServiceType_MAILA,		"MAILA" }
};

static OSStatus	RecordTypeFromArgString( const char *inString, uint16_t *outValue )
{
	OSStatus						err;
	int32_t							i32;
	const RecordType *				type;
	const RecordType * const		end = kRecordTypes + countof( kRecordTypes );
	
	for( type = kRecordTypes; type < end; ++type )
	{
		if( strcasecmp( type->name, inString ) == 0 )
		{
			*outValue = type->value;
			return( kNoErr );
		}
	}
	
	err = StringToInt32( inString, &i32 );
	require_noerr_quiet( err, exit );
	require_action_quiet( ( i32 >= 0 ) && ( i32 <= UINT16_MAX ), exit, err = kParamErr );
	
	*outValue = (uint16_t) i32;
	
exit:
	return( err );
}

//===========================================================================================================================
//	RecordClassFromArgString
//===========================================================================================================================

static OSStatus	RecordClassFromArgString( const char *inString, uint16_t *outValue )
{
	OSStatus		err;
	int32_t			i32;
	
	if( strcasecmp( inString, "IN" ) == 0 )
	{
		*outValue = kDNSServiceClass_IN;
		err = kNoErr;
		goto exit;
	}
	
	err = StringToInt32( inString, &i32 );
	require_noerr_quiet( err, exit );
	require_action_quiet( ( i32 >= 0 ) && ( i32 <= UINT16_MAX ), exit, err = kParamErr );
	
	*outValue = (uint16_t) i32;
	
exit:
	return( err );
}

//===========================================================================================================================
//	InterfaceIndexToName
//===========================================================================================================================

static char * InterfaceIndexToName( uint32_t inIfIndex, char inNameBuf[ kInterfaceNameBufLen ] )
{
	switch( inIfIndex )
	{
		case kDNSServiceInterfaceIndexAny:
			strlcpy( inNameBuf, "Any", kInterfaceNameBufLen );
			break;
		
		case kDNSServiceInterfaceIndexLocalOnly:
			strlcpy( inNameBuf, "LocalOnly", kInterfaceNameBufLen );
			break;
		
		case kDNSServiceInterfaceIndexUnicast:
			strlcpy( inNameBuf, "Unicast", kInterfaceNameBufLen );
			break;
		
		case kDNSServiceInterfaceIndexP2P:
			strlcpy( inNameBuf, "P2P", kInterfaceNameBufLen );
			break;
		
	#if( defined( kDNSServiceInterfaceIndexBLE ) )
		case kDNSServiceInterfaceIndexBLE:
			strlcpy( inNameBuf, "BLE", kInterfaceNameBufLen );
			break;
	#endif
		
		default:
		{
			const char *		name;
			
			name = if_indextoname( inIfIndex, inNameBuf );
			if( !name ) strlcpy( inNameBuf, "NO NAME", kInterfaceNameBufLen );
			break;
		}
	}
	
	return( inNameBuf );
}

//===========================================================================================================================
//	RecordTypeToString
//===========================================================================================================================

static const char *	RecordTypeToString( unsigned int inValue )
{
	const RecordType *				type;
	const RecordType * const		end = kRecordTypes + countof( kRecordTypes );
	
	for( type = kRecordTypes; type < end; ++type )
	{
		if( type->value == inValue ) return( type->name );
	}
	return( "???" );
}

//===========================================================================================================================
//	DNSMessageExtractDomainName
//===========================================================================================================================

#define IsCompressionByte( X )		( ( ( X ) & 0xC0 ) == 0xC0 )

static OSStatus
	DNSMessageExtractDomainName(
		const uint8_t *		inMsgPtr,
		size_t				inMsgLen,
		const uint8_t *		inNamePtr,
		uint8_t				inBuf[ kDomainNameLengthMax ],
		const uint8_t **	outNextPtr )
{
	OSStatus					err;
	const uint8_t *				label;
	uint8_t						labelLen;
	const uint8_t *				nextLabel;
	const uint8_t * const		msgEnd	= inMsgPtr + inMsgLen;
	uint8_t *					dst		= inBuf;
	const uint8_t * const		dstLim	= inBuf ? ( inBuf + kDomainNameLengthMax ) : NULL;
	const uint8_t *				nameEnd	= NULL;
	
	require_action( ( inNamePtr >= inMsgPtr ) && ( inNamePtr < msgEnd ), exit, err = kRangeErr );
	
	for( label = inNamePtr; ( labelLen = label[ 0 ] ) != 0; label = nextLabel )
	{
		if( labelLen <= kDomainLabelLengthMax )
		{
			nextLabel = label + 1 + labelLen;
			require_action( nextLabel < msgEnd, exit, err = kUnderrunErr );
			if( dst )
			{
				require_action( ( dstLim - dst ) > ( 1 + labelLen ), exit, err = kOverrunErr );
				memcpy( dst, label, 1 + labelLen );
				dst += ( 1 + labelLen );
			}
		}
		else if( IsCompressionByte( labelLen ) )
		{
			uint16_t		offset;
			
			require_action( ( msgEnd - label ) >= 2, exit, err = kUnderrunErr );
			if( !nameEnd )
			{
				nameEnd = label + 2;
				if( !dst ) break;
			}
			offset = (uint16_t)( ( ( label[ 0 ] & 0x3F ) << 8 ) | label[ 1 ] );
			nextLabel = inMsgPtr + offset;
			require_action( nextLabel < msgEnd, exit, err = kUnderrunErr );
			require_action( !IsCompressionByte( nextLabel[ 0 ] ), exit, err = kMalformedErr );
		}
		else
		{
			dlogassert( "Unhandled label length 0x%02X\n", labelLen );
			err = kMalformedErr;
			goto exit;
		}
	}
	
	if( dst ) *dst = 0;
	if( !nameEnd ) nameEnd = label + 1;
	
	if( outNextPtr ) *outNextPtr = nameEnd;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DNSMessageExtractDomainNameString
//===========================================================================================================================

static OSStatus
	DNSMessageExtractDomainNameString(
		const void *		inMsgPtr,
		size_t				inMsgLen,
		const void *		inNamePtr,
		char				inBuf[ kDNSServiceMaxDomainName ],
		const uint8_t **	outNextPtr )
{
	OSStatus			err;
	const uint8_t *		nextPtr;
	uint8_t				domainName[ kDomainNameLengthMax ];
	
	err = DNSMessageExtractDomainName( inMsgPtr, inMsgLen, inNamePtr, domainName, &nextPtr );
	require_noerr( err, exit );
	
	err = DomainNameToString( domainName, NULL, inBuf, NULL );
	require_noerr( err, exit );
	
	if( outNextPtr ) *outNextPtr = nextPtr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DNSMessageExtractRecord
//===========================================================================================================================

typedef struct
{
	uint8_t		type[ 2 ];
	uint8_t		class[ 2 ];
	uint8_t		ttl[ 4 ];
	uint8_t		rdLength[ 2 ];
	uint8_t		rdata[ 1 ];
	
}	DNSRecordFields;

check_compile_time( offsetof( DNSRecordFields, rdata ) == 10 );

static OSStatus
	DNSMessageExtractRecord(
		const uint8_t *		inMsgPtr,
		size_t				inMsgLen,
		const uint8_t *		inPtr,
		uint8_t				inNameBuf[ kDomainNameLengthMax ],
		uint16_t *			outType,
		uint16_t *			outClass,
		uint32_t *			outTTL,
		const uint8_t **	outRDataPtr,
		size_t *			outRDataLen,
		const uint8_t **	outPtr )
{
	OSStatus					err;
	const uint8_t * const		msgEnd = inMsgPtr + inMsgLen;
	const uint8_t *				ptr;
	const DNSRecordFields *		record;
	size_t						rdLength;
	
	err = DNSMessageExtractDomainName( inMsgPtr, inMsgLen, inPtr, inNameBuf, &ptr );
	require_noerr_quiet( err, exit );
	require_action_quiet( (size_t)( msgEnd - ptr ) >= offsetof( DNSRecordFields, rdata ), exit, err = kUnderrunErr );
	
	record = (DNSRecordFields *) ptr;
	rdLength = ReadBig16( record->rdLength );
	require_action_quiet( (size_t)( msgEnd - record->rdata ) >= rdLength , exit, err = kUnderrunErr );
	
	if( outType )		*outType		= ReadBig16( record->type );
	if( outClass )		*outClass		= ReadBig16( record->class );
	if( outTTL )		*outTTL			= ReadBig32( record->ttl );
	if( outRDataPtr )	*outRDataPtr	= record->rdata;
	if( outRDataLen )	*outRDataLen	= rdLength;
	if( outPtr )		*outPtr			= record->rdata + rdLength;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DNSMessageGetAnswerSection
//===========================================================================================================================

static OSStatus	DNSMessageGetAnswerSection( const uint8_t *inMsgPtr, size_t inMsgLen, const uint8_t **outPtr )
{
	OSStatus					err;
	const uint8_t * const		msgEnd	= inMsgPtr + inMsgLen;
	unsigned int				questionCount, i;
	const DNSHeader *			hdr;
	const uint8_t *				ptr;
	
	require_action_quiet( inMsgLen >= kDNSHeaderLength, exit, err = kSizeErr );
	
	hdr = (DNSHeader *) inMsgPtr;
	questionCount = DNSHeaderGetQuestionCount( hdr );
	
	ptr = (uint8_t *)( hdr + 1 );
	for( i = 0; i < questionCount; ++i )
	{
		err = DNSMessageExtractDomainName( inMsgPtr, inMsgLen, ptr, NULL, &ptr );
		require_noerr( err, exit );
		require_action_quiet( ( msgEnd - ptr ) >= 4, exit, err = kUnderrunErr );
		ptr += 4;
	}
	
	if( outPtr ) *outPtr = ptr;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DNSRecordDataToString
//===========================================================================================================================

static OSStatus
	DNSRecordDataToString(
		const void *	inRDataPtr,
		size_t			inRDataLen,
		unsigned int	inRDataType,
		const void *	inMsgPtr,
		size_t			inMsgLen,
		char **			outString )
{
	OSStatus					err;
	const uint8_t * const		rdataPtr = (uint8_t *) inRDataPtr;
	const uint8_t * const		rdataEnd = rdataPtr + inRDataLen;
	char *						rdataStr;
	const uint8_t *				ptr;
	int							n;
	char						domainNameStr[ kDNSServiceMaxDomainName ];
	
	rdataStr = NULL;
	if( inRDataType == kDNSServiceType_A )
	{
		require_action_quiet( inRDataLen == 4, exit, err = kMalformedErr );
		
		ASPrintF( &rdataStr, "%.4a", rdataPtr );
		require_action( rdataStr, exit, err = kNoMemoryErr );
	}
	else if( inRDataType == kDNSServiceType_AAAA )
	{
		require_action_quiet( inRDataLen == 16, exit, err = kMalformedErr );
		
		ASPrintF( &rdataStr, "%.16a", rdataPtr );
		require_action( rdataStr, exit, err = kNoMemoryErr );
	}
	else if( ( inRDataType == kDNSServiceType_PTR ) || ( inRDataType == kDNSServiceType_CNAME ) ||
			( inRDataType == kDNSServiceType_NS ) )
	{
		if( inMsgPtr )
		{
			err = DNSMessageExtractDomainNameString( inMsgPtr, inMsgLen, rdataPtr, domainNameStr, NULL );
			require_noerr( err, exit );
		}
		else
		{
			err = DomainNameToString( rdataPtr, rdataEnd, domainNameStr, NULL );
			require_noerr( err, exit );
		}
		
		rdataStr = strdup( domainNameStr );
		require_action( rdataStr, exit, err = kNoMemoryErr );
	}
	else if( inRDataType == kDNSServiceType_SRV )
	{
		uint16_t			priority, weight, port;
		const uint8_t *		target;
		
		require_action_quiet( ( rdataPtr + 6 ) < rdataEnd, exit, err = kMalformedErr );
		
		priority	= ReadBig16( rdataPtr );
		weight		= ReadBig16( rdataPtr + 2 );
		port		= ReadBig16( rdataPtr + 4 );
		target		= rdataPtr + 6;
		
		if( inMsgPtr )
		{
			err = DNSMessageExtractDomainNameString( inMsgPtr, inMsgLen, target, domainNameStr, NULL );
			require_noerr( err, exit );
		}
		else
		{
			err = DomainNameToString( target, rdataEnd, domainNameStr, NULL );
			require_noerr( err, exit );
		}
		
		ASPrintF( &rdataStr, "%u %u %u %s", priority, weight, port, domainNameStr );
		require_action( rdataStr, exit, err = kNoMemoryErr );
	}
	else if( inRDataType == kDNSServiceType_TXT )
	{
		require_action_quiet( inRDataLen > 0, exit, err = kMalformedErr );
		
		if( inRDataLen == 1 )
		{
			ASPrintF( &rdataStr, "%#H", rdataPtr, (int) inRDataLen, INT_MAX );
			require_action( rdataStr, exit, err = kNoMemoryErr );
		}
		else
		{
			ASPrintF( &rdataStr, "%#{txt}", rdataPtr, inRDataLen );
			require_action( rdataStr, exit, err = kNoMemoryErr );
		}
	}
	else if( inRDataType == kDNSServiceType_SOA )
	{
		uint32_t		serial, refresh, retry, expire, minimum;
		
		if( inMsgPtr )
		{
			err = DNSMessageExtractDomainNameString( inMsgPtr, inMsgLen, rdataPtr, domainNameStr, &ptr );
			require_noerr( err, exit );
			
			require_action_quiet( ptr < rdataEnd, exit, err = kMalformedErr );
			
			rdataStr = strdup( domainNameStr );
			require_action( rdataStr, exit, err = kNoMemoryErr );
			
			err = DNSMessageExtractDomainNameString( inMsgPtr, inMsgLen, ptr, domainNameStr, &ptr );
			require_noerr( err, exit );
		}
		else
		{
			err = DomainNameToString( rdataPtr, rdataEnd, domainNameStr, &ptr );
			require_noerr( err, exit );
			
			rdataStr = strdup( domainNameStr );
			require_action( rdataStr, exit, err = kNoMemoryErr );
			
			err = DomainNameToString( ptr, rdataEnd, domainNameStr, &ptr );
			require_noerr( err, exit );
		}
		
		require_action_quiet( ( ptr + 20 ) == rdataEnd, exit, err = kMalformedErr );
		
		serial	= ReadBig32( ptr );
		refresh	= ReadBig32( ptr +  4 );
		retry	= ReadBig32( ptr +  8 );
		expire	= ReadBig32( ptr + 12 );
		minimum	= ReadBig32( ptr + 16 );
		
		n = AppendPrintF( &rdataStr, " %s %u %u %u %u %u\n", domainNameStr, serial, refresh, retry, expire, minimum );
		require_action( n > 0, exit, err = kUnknownErr );
	}
	else if( inRDataType == kDNSServiceType_NSEC )
	{
		unsigned int		windowBlock, bitmapLen, i, recordType;
		const uint8_t *		bitmapPtr;
		
		if( inMsgPtr )
		{
			err = DNSMessageExtractDomainNameString( inMsgPtr, inMsgLen, rdataPtr, domainNameStr, &ptr );
			require_noerr( err, exit );
		}
		else
		{
			err = DomainNameToString( rdataPtr, rdataEnd, domainNameStr, &ptr );
			require_noerr( err, exit );
		}
		
		require_action_quiet( ptr < rdataEnd, exit, err = kMalformedErr );
		
		rdataStr = strdup( domainNameStr );
		require_action( rdataStr, exit, err = kNoMemoryErr );
		
		for( ; ptr < rdataEnd; ptr += ( 2 + bitmapLen ) )
		{
			require_action_quiet( ( ptr + 2 ) < rdataEnd, exit, err = kMalformedErr );
			
			windowBlock	= ptr[ 0 ];
			bitmapLen	= ptr[ 1 ];
			bitmapPtr	= &ptr[ 2 ];
			
			require_action_quiet( ( bitmapLen >= 1 ) && ( bitmapLen <= 32 ) , exit, err = kMalformedErr );
			require_action_quiet( ( bitmapPtr + bitmapLen ) <= rdataEnd, exit, err = kMalformedErr );
			
			for( i = 0; i < BitArray_MaxBits( bitmapLen ); ++i )
			{
				if( BitArray_GetBit( bitmapPtr, bitmapLen, i ) )
				{
					recordType = ( windowBlock * 256 ) + i;
					n = AppendPrintF( &rdataStr, " %s", RecordTypeToString( recordType ) );
					require_action( n > 0, exit, err = kUnknownErr );
				}
			}
		}
	}
	else if( inRDataType == kDNSServiceType_MX )
	{
		uint16_t			preference;
		const uint8_t *		exchange;
		
		require_action_quiet( ( rdataPtr + 2 ) < rdataEnd, exit, err = kMalformedErr );
		
		preference	= ReadBig16( rdataPtr );
		exchange	= &rdataPtr[ 2 ];
		
		if( inMsgPtr )
		{
			err = DNSMessageExtractDomainNameString( inMsgPtr, inMsgLen, exchange, domainNameStr, NULL );
			require_noerr( err, exit );
		}
		else
		{
			err = DomainNameToString( exchange, rdataEnd, domainNameStr, NULL );
			require_noerr( err, exit );
		}
		
		n = ASPrintF( &rdataStr, "%u %s", preference, domainNameStr );
		require_action( n > 0, exit, err = kUnknownErr );
	}
	else
	{
		err = kNotHandledErr;
		goto exit;
	}
	
	check( rdataStr );
	*outString = rdataStr;
	rdataStr = NULL;
	err = kNoErr;
	
exit:
	FreeNullSafe( rdataStr );
	return( err );
}

//===========================================================================================================================
//	DomainNameAppendString
//===========================================================================================================================

static OSStatus
	DomainNameAppendString(
		uint8_t			inDomainName[ kDomainNameLengthMax ],
		const char *	inString,
		uint8_t **		outEndPtr )
{
	OSStatus					err;
	const char *				src;
	uint8_t *					dst;
	const uint8_t * const		nameLimit = inDomainName + kDomainNameLengthMax;
	
	// Find the root label.
	
	for( dst = inDomainName; ( dst < nameLimit ) && *dst; dst += ( 1 + *dst ) ) {}
	require_action_quiet( dst < nameLimit, exit, err = kMalformedErr );
	
	// Append the string's labels one label at a time.
	
	src = inString;
	while( *src )
	{
		uint8_t * const				label		= dst++;
		const uint8_t * const		labelLimit	= Min( dst + kDomainLabelLengthMax, nameLimit - 1 );
		
		// If the first character is a label separator, then the label is empty. Empty non-root labels are not allowed.
		
		require_action_quiet( *src != '.', exit, err = kMalformedErr );
		
		// Write the label characters until the end of the label, a separator or NUL character, is encountered, or until no
		// more space is available.
		
		while( ( *src != '.' ) && ( *src != '\0' ) && ( dst < labelLimit ) )
		{
			uint8_t		value;
			
			value = (uint8_t) *src++;
			if( value == '\\' )
			{
				if( *src == '\0' ) break;
				value = (uint8_t) *src++;
				if( isdigit_safe( value ) && isdigit_safe( src[ 0 ] ) && isdigit_safe( src[ 1 ] ) )
				{
					int		decimalValue;
					
					decimalValue = ( ( value - '0' ) * 100 ) + ( ( src[ 0 ] - '0' ) * 10 ) + ( src[ 1 ] - '0' );
					if( decimalValue <= 255 )
					{
						value = (uint8_t) decimalValue;
						src += 2;
					}
				}
			}
			*dst++ = value;
		}
		if( ( *src == '.' ) || ( *src == '\0' ) )
		{
			label[ 0 ] = (uint8_t)( dst - &label[ 1 ] );	// Write the label length.
			if( *src == '.' ) ++src;						// Advance the pointer past the label separator.
		}
		else
		{
			label[ 0 ] = 0;
			err = kOverrunErr;
			goto exit;
		}
	}
	
	*dst++ = 0;	// Write the empty root label.
	if( outEndPtr ) *outEndPtr = dst;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DomainNameEqual
//===========================================================================================================================

static Boolean	DomainNameEqual( const uint8_t *inName1, const uint8_t *inName2 )
{
	const uint8_t *		p1 = inName1;
	const uint8_t *		p2 = inName2;
	unsigned int		len;
	
	for( ;; )
	{
		if( ( len = *p1++ ) != *p2++ ) return( false );
		if( len == 0 ) break;
		for( ; len > 0; ++p1, ++p2, --len )
		{
			if( tolower_safe( *p1 ) != tolower_safe( *p2 ) ) return( false );
		}
	}
	return( true );
}

//===========================================================================================================================
//	DomainNameToString
//===========================================================================================================================

static OSStatus
	DomainNameToString(
		const uint8_t *		inDomainName,
		const uint8_t *		inEnd,
		char				inBuf[ kDNSServiceMaxDomainName ],
		const uint8_t **	outNextPtr )
{
	OSStatus			err;
	const uint8_t *		label;
	uint8_t				labelLen;
	const uint8_t *		nextLabel;
	char *				dst;
	const uint8_t *		src;
	
	require_action( !inEnd || ( inDomainName < inEnd ), exit, err = kUnderrunErr );
	
	// Convert each label up until the root label, i.e., the zero-length label.
	
	dst = inBuf;
	for( label = inDomainName; ( labelLen = label[ 0 ] ) != 0; label = nextLabel )
	{
		require_action( labelLen <= kDomainLabelLengthMax, exit, err = kMalformedErr );
		
		nextLabel = &label[ 1 ] + labelLen;
		require_action( ( nextLabel - inDomainName ) < kDomainNameLengthMax, exit, err = kMalformedErr );
		require_action( !inEnd || ( nextLabel < inEnd ), exit, err = kUnderrunErr );
		
		for( src = &label[ 1 ]; src < nextLabel; ++src )
		{
			if( isprint_safe( *src ) )
			{
				if( ( *src == '.' ) || ( *src == '\\' ) ||  ( *src == ' ' ) ) *dst++ = '\\';
				*dst++ = (char) *src;
			}
			else
			{
				*dst++ = '\\';
				*dst++ = '0' + (   *src / 100 );
				*dst++ = '0' + ( ( *src /  10 ) % 10 );
				*dst++ = '0' + (   *src         % 10 );
			}
		}
		*dst++ = '.';
	}
	
	// At this point, label points to the root label.
	// If the root label was the only label, then write a dot for it.
	
	if( label == inDomainName ) *dst++ = '.';
	*dst = '\0';
	if( outNextPtr ) *outNextPtr = label + 1;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	PrintDNSMessage
//===========================================================================================================================

#define DNSFlagsOpCodeToString( X ) (					\
	( (X) == kDNSOpCode_Query )			? "Query"	:	\
	( (X) == kDNSOpCode_InverseQuery )	? "IQuery"	:	\
	( (X) == kDNSOpCode_Status )		? "Status"	:	\
	( (X) == kDNSOpCode_Notify )		? "Notify"	:	\
	( (X) == kDNSOpCode_Update )		? "Update"	:	\
										  "Unassigned" )

#define DNSFlagsRCodeToString( X ) (						\
	( (X) == kDNSRCode_NoError )		? "NoError"		:	\
	( (X) == kDNSRCode_FormatError )	? "FormErr"		:	\
	( (X) == kDNSRCode_ServerFailure )	? "ServFail"	:	\
	( (X) == kDNSRCode_NXDomain )		? "NXDomain"	:	\
	( (X) == kDNSRCode_NotImplemented )	? "NotImp"		:	\
	( (X) == kDNSRCode_Refused )		? "Refused"		:	\
										  "???" )

#define DNSFlagsGetOpCode( X )		( ( (X) >> 11 ) & 0x0F )
#define DNSFlagsGetRCode( X )		(   (X)         & 0x0F )

static OSStatus	PrintDNSMessage( const uint8_t *inMsgPtr, size_t inMsgLen, const Boolean inIsMDNS, const Boolean inPrintRaw )
{
	OSStatus					err;
	const DNSHeader *			hdr;
	const uint8_t * const		msgEnd = inMsgPtr + inMsgLen;
	const uint8_t *				ptr;
	unsigned int				id, flags, opcode, rcode;
	unsigned int				questionCount, answerCount, authorityCount, additionalCount, i, totalRRCount;
	char						nameStr[ kDNSServiceMaxDomainName ];
	
	require_action_quiet( inMsgLen >= kDNSHeaderLength, exit, err = kSizeErr );
	
	hdr				= (DNSHeader *) inMsgPtr;
	id				= DNSHeaderGetID( hdr );
	flags			= DNSHeaderGetFlags( hdr );
	questionCount	= DNSHeaderGetQuestionCount( hdr );
	answerCount		= DNSHeaderGetAnswerCount( hdr );
	authorityCount	= DNSHeaderGetAuthorityCount( hdr );
	additionalCount	= DNSHeaderGetAdditionalCount( hdr );
	opcode			= DNSFlagsGetOpCode( flags );
	rcode			= DNSFlagsGetRCode( flags );
	
	FPrintF( stdout, "ID:               0x%04X\n", id );
	FPrintF( stdout, "Flags:            0x%04X %c/%s %cAA%cTC%cRD%cRA %s\n",
		flags,
		( flags & kDNSHeaderFlag_Response )				? 'R' : 'Q', DNSFlagsOpCodeToString( opcode ),
		( flags & kDNSHeaderFlag_AuthAnswer )			? ' ' : '!',
		( flags & kDNSHeaderFlag_Truncation )			? ' ' : '!',
		( flags & kDNSHeaderFlag_RecursionDesired )		? ' ' : '!',
		( flags & kDNSHeaderFlag_RecursionAvailable )	? ' ' : '!',
		DNSFlagsRCodeToString( rcode ) );
	FPrintF( stdout, "Question count:   %u\n", questionCount );
	FPrintF( stdout, "Answer count:     %u\n", answerCount );
	FPrintF( stdout, "Authority count:  %u\n", authorityCount );
	FPrintF( stdout, "Additional count: %u\n", additionalCount );
	
	ptr = (uint8_t *)( hdr + 1 );
	for( i = 0; i < questionCount; ++i )
	{
		unsigned int		qType, qClass;
		Boolean				isQU;
		
		err = DNSMessageExtractDomainNameString( inMsgPtr, inMsgLen, ptr, nameStr, &ptr );
		require_noerr( err, exit );
		
		if( ( msgEnd - ptr ) < 4 )
		{
			err = kUnderrunErr;
			goto exit;
		}
		
		qType = ReadBig16( ptr );
		ptr += 2;
		qClass = ReadBig16( ptr );
		ptr += 2;
		
		isQU = ( inIsMDNS && ( qClass & kQClassUnicastResponseBit ) ) ? true : false;
		if( inIsMDNS ) qClass &= ~kQClassUnicastResponseBit;
		
		if( i == 0 ) FPrintF( stdout, "\nQUESTION SECTION\n" );
		
		FPrintF( stdout, "%s %2s %?2s%?2u %-5s\n",
			nameStr, inIsMDNS ? ( isQU ? "QU" : "QM" ) : "",
			( qClass == kDNSServiceClass_IN ), "IN", ( qClass != kDNSServiceClass_IN ), qClass,
			RecordTypeToString( qType ) );
	}
	
	totalRRCount = answerCount + authorityCount + additionalCount;
	for( i = 0; i < totalRRCount; ++i )
	{
		uint16_t			type;
		uint16_t			class;
		uint32_t			ttl;
		const uint8_t *		rdataPtr;
		size_t				rdataLen;
		char *				rdataStr;
		Boolean				cacheFlush;
		uint8_t				name[ kDomainNameLengthMax ];
		
		err = DNSMessageExtractRecord( inMsgPtr, inMsgLen, ptr, name, &type, &class, &ttl, &rdataPtr, &rdataLen, &ptr );
		require_noerr( err, exit );
		
		err = DomainNameToString( name, NULL, nameStr, NULL );
		require_noerr( err, exit );
		
		cacheFlush = ( inIsMDNS && ( class & kRRClassCacheFlushBit ) ) ? true : false;
		if( inIsMDNS ) class &= ~kRRClassCacheFlushBit;
		
		rdataStr = NULL;
		if( !inPrintRaw ) DNSRecordDataToString( rdataPtr, rdataLen, type, inMsgPtr, inMsgLen, &rdataStr );
		if( !rdataStr )
		{
			ASPrintF( &rdataStr, "%#H", rdataPtr, (int) rdataLen, INT_MAX );
			require_action( rdataStr, exit, err = kNoMemoryErr );
		}
		
		if(      answerCount     && ( i ==   0                              ) ) FPrintF( stdout, "\nANSWER SECTION\n" );
		else if( authorityCount  && ( i ==   answerCount                    ) ) FPrintF( stdout, "\nAUTHORITY SECTION\n" );
		else if( additionalCount && ( i == ( answerCount + authorityCount ) ) ) FPrintF( stdout, "\nADDITIONAL SECTION\n" );
		
		FPrintF( stdout, "%-42s %6u %2s %?2s%?2u %-5s %s\n",
			nameStr, ttl, cacheFlush ? "CF" : "",
			( class == kDNSServiceClass_IN ), "IN", ( class != kDNSServiceClass_IN ), class,
			RecordTypeToString( type ), rdataStr );
		free( rdataStr );
	}
	FPrintF( stdout, "\n" );
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	WriteDNSQueryMessage
//===========================================================================================================================

static OSStatus
	WriteDNSQueryMessage(
		uint8_t			inMsg[ kDNSQueryMessageMaxLen ],
		uint16_t		inMsgID,
		uint16_t		inFlags,
		const char *	inQName,
		uint16_t		inQType,
		uint16_t		inQClass,
		size_t *		outMsgLen )
{
	OSStatus				err;
	DNSHeader * const		hdr = (DNSHeader *) inMsg;
	uint8_t *				ptr;
	size_t					msgLen;
	
	WriteBig16( hdr->id,				inMsgID );
	WriteBig16( hdr->flags,				inFlags );
	WriteBig16( hdr->questionCount,		1 );
	WriteBig16( hdr->answerCount,		0 );
	WriteBig16( hdr->authorityCount,	0 );
	WriteBig16( hdr->additionalCount,	0 );
	
	ptr = (uint8_t *)( hdr + 1 );
	ptr[ 0 ] = 0;
	err = DomainNameAppendString( ptr, inQName, &ptr );
	require_noerr_quiet( err, exit );
	
	WriteBig16( ptr, inQType );
	ptr += 2;
	WriteBig16( ptr, inQClass );
	ptr += 2;
	msgLen = (size_t)( ptr - inMsg );
	check( msgLen <= kDNSQueryMessageMaxLen );
	
	if( outMsgLen ) *outMsgLen = msgLen;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DispatchSignalSourceCreate
//===========================================================================================================================

static OSStatus
	DispatchSignalSourceCreate(
		int					inSignal,
		DispatchHandler		inEventHandler,
		void *				inContext,
		dispatch_source_t *	outSource )
{
	OSStatus				err;
	dispatch_source_t		source;
	
	source = dispatch_source_create( DISPATCH_SOURCE_TYPE_SIGNAL, (uintptr_t) inSignal, 0, dispatch_get_main_queue() );
	require_action( source, exit, err = kUnknownErr );
	
	dispatch_set_context( source, inContext );
	dispatch_source_set_event_handler_f( source, inEventHandler );
	
	*outSource = source;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DispatchReadSourceCreate
//===========================================================================================================================

static OSStatus
	DispatchReadSourceCreate(
		SocketRef			inSock,
		DispatchHandler		inEventHandler,
		DispatchHandler		inCancelHandler,
		void *				inContext,
		dispatch_source_t *	outSource )
{
	OSStatus				err;
	dispatch_source_t		source;
	
	source = dispatch_source_create( DISPATCH_SOURCE_TYPE_READ, (uintptr_t) inSock, 0, dispatch_get_main_queue() );
	require_action( source, exit, err = kUnknownErr );
	
	dispatch_set_context( source, inContext );
	dispatch_source_set_event_handler_f( source, inEventHandler );
	dispatch_source_set_cancel_handler_f( source, inCancelHandler );
	
	*outSource = source;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	DispatchTimerCreate
//===========================================================================================================================

static OSStatus
	DispatchTimerCreate(
		dispatch_time_t		inStart,
		uint64_t			inIntervalNs,
		uint64_t			inLeewayNs,
		DispatchHandler		inEventHandler,
		DispatchHandler		inCancelHandler,
		void *				inContext,
		dispatch_source_t *	outTimer )
{
	OSStatus				err;
	dispatch_source_t		timer;
	
	timer = dispatch_source_create( DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue() );
	require_action( timer, exit, err = kUnknownErr );
	
	dispatch_source_set_timer( timer, inStart, inIntervalNs, inLeewayNs );
	dispatch_set_context( timer, inContext );
	dispatch_source_set_event_handler_f( timer, inEventHandler );
	dispatch_source_set_cancel_handler_f( timer, inCancelHandler );
	
	*outTimer = timer;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	ServiceTypeDescription
//===========================================================================================================================

typedef struct
{
	const char *		name;			// Name of the service type in two-label "_service._proto" format.
	const char *		description;	// Description of the service type.
	
}	ServiceType;

// A Non-comprehensive table of DNS-SD service types

static const ServiceType		kServiceTypes[] =
{
	{ "_acp-sync._tcp",			"AirPort Base Station Sync" },
	{ "_adisk._tcp",			"Automatic Disk Discovery" },
	{ "_afpovertcp._tcp",		"Apple File Sharing" },
	{ "_airdrop._tcp",			"AirDrop" },
	{ "_airplay._tcp",			"AirPlay" },
	{ "_airport._tcp",			"AirPort Base Station" },
	{ "_daap._tcp",				"Digital Audio Access Protocol (iTunes)" },
	{ "_eppc._tcp",				"Remote AppleEvents" },
	{ "_ftp._tcp",				"File Transfer Protocol" },
	{ "_home-sharing._tcp",		"Home Sharing" },
	{ "_homekit._tcp",			"HomeKit" },
	{ "_http._tcp",				"World Wide Web HTML-over-HTTP" },
	{ "_https._tcp",			"HTTP over SSL/TLS" },
	{ "_ipp._tcp",				"Internet Printing Protocol" },
	{ "_ldap._tcp",				"Lightweight Directory Access Protocol" },
	{ "_mediaremotetv._tcp",	"Media Remote" },
	{ "_net-assistant._tcp",	"Apple Remote Desktop" },
	{ "_od-master._tcp",		"OpenDirectory Master" },
	{ "_nfs._tcp",				"Network File System" },
	{ "_presence._tcp",			"Peer-to-peer messaging / Link-Local Messaging" },
	{ "_pdl-datastream._tcp",	"Printer Page Description Language Data Stream" },
	{ "_raop._tcp",				"Remote Audio Output Protocol" },
	{ "_rfb._tcp",				"Remote Frame Buffer" },
	{ "_scanner._tcp",			"Bonjour Scanning" },
	{ "_smb._tcp",				"Server Message Block over TCP/IP" },
	{ "_sftp-ssh._tcp",			"Secure File Transfer Protocol over SSH" },
	{ "_sleep-proxy._udp",		"Sleep Proxy Server" },
	{ "_ssh._tcp",				"SSH Remote Login Protocol" },
	{ "_teleport._tcp",			"teleport" },
	{ "_tftp._tcp",				"Trivial File Transfer Protocol" },
	{ "_workstation._tcp",		"Workgroup Manager" },
	{ "_webdav._tcp",			"World Wide Web Distributed Authoring and Versioning (WebDAV)" },
	{ "_webdavs._tcp",			"WebDAV over SSL/TLS" }
};

static const char *	ServiceTypeDescription( const char *inName )
{
	const ServiceType *				serviceType;
	const ServiceType * const		end = kServiceTypes + countof( kServiceTypes );
	
	for( serviceType = kServiceTypes; serviceType < end; ++serviceType )
	{
		if( strcasecmp( inName, serviceType->name ) == 0 ) return( serviceType->description );
	}
	return( NULL );
}

//===========================================================================================================================
//	SocketContextCancelHandler
//===========================================================================================================================

static void	SocketContextCancelHandler( void *inContext )
{
	SocketContext * const		context = (SocketContext *) inContext;
	
	ForgetSocket( &context->sock );
	free( context );
}

//===========================================================================================================================
//	StringToInt32
//===========================================================================================================================

static OSStatus	StringToInt32( const char *inString, int32_t *outValue )
{
	OSStatus		err;
	long			value;
	char *			endPtr;
	
	value = strtol( inString, &endPtr, 0 );
	require_action_quiet( ( *endPtr == '\0' ) && ( endPtr != inString ), exit, err = kParamErr );
	require_action_quiet( ( value >= INT32_MIN ) && ( value <= INT32_MAX ), exit, err = kRangeErr );
	
	*outValue = (int32_t) value;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	StringToUInt32
//===========================================================================================================================

static OSStatus	StringToUInt32( const char *inString, uint32_t *outValue )
{
	OSStatus		err;
	uint32_t		value;
	char *			endPtr;
	
	value = (uint32_t) strtol( inString, &endPtr, 0 );
	require_action_quiet( ( *endPtr == '\0' ) && ( endPtr != inString ), exit, err = kParamErr );
	
	*outValue = value;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	SocketWriteAll
//
//	Note: This was copied from CoreUtils because the SocketWriteAll function is currently not exported in the framework.
//===========================================================================================================================

OSStatus	SocketWriteAll( SocketRef inSock, const void *inData, size_t inSize, int32_t inTimeoutSecs )
{
	OSStatus			err;
	const uint8_t *		src;
	const uint8_t *		end;
	fd_set				writeSet;
	struct timeval		timeout;
	ssize_t				n;
	
	FD_ZERO( &writeSet );
	src = (const uint8_t *) inData;
	end = src + inSize;
	while( src < end )
	{
		FD_SET( inSock, &writeSet );
		timeout.tv_sec 	= inTimeoutSecs;
		timeout.tv_usec = 0;
		n = select( (int)( inSock + 1 ), NULL, &writeSet, NULL, &timeout );
		if( n == 0 ) { err = kTimeoutErr; goto exit; }
		err = map_socket_value_errno( inSock, n > 0, n );
		require_noerr( err, exit );
		
		n = send( inSock, (char *) src, (size_t)( end - src ), 0 );
		err = map_socket_value_errno( inSock, n >= 0, n );
		if( err == EINTR ) continue;
		require_noerr( err, exit );
		
		src += n;
	}
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	ParseEscapedString
//
//	Note: This was copied from CoreUtils because the ParseEscapedString function is currently not exported in the framework.
//===========================================================================================================================

OSStatus
	ParseEscapedString( 
		const char *	inSrc, 
		const char *	inEnd, 
		char			inDelimiter, 
		char *			inBuf, 
		size_t			inMaxLen, 
		size_t *		outCopiedLen, 
		size_t *		outTotalLen, 
		const char **	outSrc )
{
	OSStatus		err;
	char			c;
	char *			dst;
	char *			lim;
	size_t			len;
	
	dst = inBuf;
	lim = dst + ( ( inMaxLen > 0 ) ? ( inMaxLen - 1 ) : 0 ); // Leave room for null terminator.
	len = 0;
	while( ( inSrc < inEnd ) && ( ( c = *inSrc++ ) != inDelimiter ) )
	{
		if( c == '\\' )
		{
			require_action_quiet( inSrc < inEnd, exit, err = kUnderrunErr );
			c = *inSrc++;
		}
		if( dst < lim )
		{
			if( inBuf ) *dst = c;
			++dst;
		}
		++len;
	}
	if( inBuf && ( inMaxLen > 0 ) ) *dst = '\0';
	err = kNoErr;
	
exit:
	if( outCopiedLen )	*outCopiedLen	= (size_t)( dst - inBuf );
	if( outTotalLen )	*outTotalLen	= len;
	if( outSrc )		*outSrc			= inSrc;
	return( err );
}

//===========================================================================================================================
//	ParseIPv4Address
//
//	Warning: "inBuffer" may be modified even in error cases.
//
//	Note: This was copied from CoreUtils because the StringToIPv4Address function is currently not exported in the framework.
//===========================================================================================================================

static OSStatus	ParseIPv4Address( const char *inStr, uint8_t inBuffer[ 4 ], const char **outStr )
{
	OSStatus		err;
	uint8_t *		dst;
	int				segments;
	int				sawDigit;
	int				c;
	int				v;
	
	check( inBuffer );
	check( outStr );
	
	dst		 = inBuffer;
	*dst	 = 0;
	sawDigit = 0;
	segments = 0;
	for( ; ( c = *inStr ) != '\0'; ++inStr )
	{
		if( isdigit_safe( c ) )
		{
			v = ( *dst * 10 ) + ( c - '0' );
			require_action_quiet( v <= 255, exit, err = kRangeErr );
			*dst = (uint8_t) v;
			if( !sawDigit )
			{
				++segments;
				require_action_quiet( segments <= 4, exit, err = kOverrunErr );
				sawDigit = 1;
			}
		}
		else if( ( c == '.' ) && sawDigit )
		{
			require_action_quiet( segments < 4, exit, err = kMalformedErr );
			*++dst = 0;
			sawDigit = 0;
		}
		else
		{
			break;
		}
	}
	require_action_quiet( segments == 4, exit, err = kUnderrunErr );
	
	*outStr = inStr;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	StringToIPv4Address
//
//	Note: This was copied from CoreUtils because the StringToIPv4Address function is currently not exported in the framework.
//===========================================================================================================================

OSStatus
	StringToIPv4Address( 
		const char *			inStr, 
		StringToIPAddressFlags	inFlags, 
		uint32_t *				outIP, 
		int *					outPort, 
		uint32_t *				outSubnet, 
		uint32_t *				outRouter, 
		const char **			outStr )
{
	OSStatus			err;
	uint8_t				buf[ 4 ];
	int					c;
	uint32_t			ip;
	int					hasPort;
	int					port;
	int					hasPrefix;
	int					prefix;
	uint32_t			subnetMask;
	uint32_t			router;
	
	require_action( inStr, exit, err = kParamErr );
	
	// Parse the address-only part of the address (e.g. "1.2.3.4").
	
	err = ParseIPv4Address( inStr, buf, &inStr );
	require_noerr_quiet( err, exit );
	ip = (uint32_t)( ( buf[ 0 ] << 24 ) | ( buf[ 1 ] << 16 ) | ( buf[ 2 ] << 8 ) | buf[ 3 ] );
	c = *inStr;
	
	// Parse the port (if any).
	
	hasPort = 0;
	port    = 0;
	if( c == ':' )
	{
		require_action_quiet( !( inFlags & kStringToIPAddressFlagsNoPort ), exit, err = kUnexpectedErr );
		while( ( ( c = *( ++inStr ) ) != '\0' ) && ( ( c >= '0' ) && ( c <= '9' ) ) ) port = ( port * 10 ) + ( c - '0' );
		require_action_quiet( port <= 65535, exit, err = kRangeErr );
		hasPort = 1;
	}
	
	// Parse the prefix length (if any).
	
	hasPrefix  = 0;
	prefix     = 0;
	subnetMask = 0;
	router     = 0;
	if( c == '/' )
	{
		require_action_quiet( !( inFlags & kStringToIPAddressFlagsNoPrefix ), exit, err = kUnexpectedErr );
		while( ( ( c = *( ++inStr ) ) != '\0' ) && ( ( c >= '0' ) && ( c <= '9' ) ) ) prefix = ( prefix * 10 ) + ( c - '0' );
		require_action_quiet( ( prefix >= 0 ) && ( prefix <= 32 ), exit, err = kRangeErr );
		hasPrefix = 1;
		
		subnetMask = ( prefix > 0 ) ? ( UINT32_C( 0xFFFFFFFF ) << ( 32 - prefix ) ) : 0;
		router	   = ( ip & subnetMask ) | 1;
	}
	
	// Return the results. Only fill in port/prefix/router results if the info was found to allow for defaults.
	
	if( outIP )					 *outIP		= ip;
	if( outPort   && hasPort )	 *outPort	= port;
	if( outSubnet && hasPrefix ) *outSubnet	= subnetMask;
	if( outRouter && hasPrefix ) *outRouter	= router;
	if( outStr )				 *outStr	= inStr;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	ParseIPv6Address
//
//	Note: Parsed according to the rules specified in RFC 3513.
//	Warning: "inBuffer" may be modified even in error cases.
//
//	Note: This was copied from CoreUtils because the StringToIPv6Address function is currently not exported in the framework.
//===========================================================================================================================

static OSStatus	ParseIPv6Address( const char *inStr, int inAllowV4Mapped, uint8_t inBuffer[ 16 ], const char **outStr )
{
													// Table to map uppercase hex characters - '0' to their numeric values.
													// 0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?  @  A   B   C   D   E   F
	static const uint8_t		kASCIItoHexTable[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };
	OSStatus					err;
	const char *				ptr;
	uint8_t *					dst;
	uint8_t *					lim;
	uint8_t *					colonPtr;
	int							c;
	int							sawDigit;
	unsigned int				v;
	int							i;
	int							n;
	
	// Pre-zero the address to simplify handling of compressed addresses (e.g. "::1").
	
	for( i = 0; i < 16; ++i ) inBuffer[ i ] = 0;
	
	// Special case leading :: (e.g. "::1") to simplify processing later.
	
	if( *inStr == ':' )
	{
		++inStr;
		require_action_quiet( *inStr == ':', exit, err = kMalformedErr );
	}
	
	// Parse the address.
	
	ptr		 = inStr;
	dst		 = inBuffer;
	lim		 = dst + 16;
	colonPtr = NULL;
	sawDigit = 0;
	v		 = 0;
	while( ( ( c = *inStr++ ) != '\0' ) && ( c != '%' ) && ( c != '/' ) && ( c != ']' ) )
	{
		if(   ( c >= 'a' ) && ( c <= 'f' ) ) c -= ( 'a' - 'A' );
		if( ( ( c >= '0' ) && ( c <= '9' ) ) || ( ( c >= 'A' ) && ( c <= 'F' ) ) )
		{
			c -= '0';
			check( c < (int) countof( kASCIItoHexTable ) );
			v = ( v << 4 ) | kASCIItoHexTable[ c ];
			require_action_quiet( v <= 0xFFFF, exit, err = kRangeErr );
			sawDigit = 1;
			continue;
		}
		if( c == ':' )
		{
			ptr = inStr;
			if( !sawDigit )
			{
				require_action_quiet( !colonPtr, exit, err = kMalformedErr );
				colonPtr = dst;
				continue;
			}
			require_action_quiet( *inStr != '\0', exit, err = kUnderrunErr );
			require_action_quiet( ( dst + 2 ) <= lim, exit, err = kOverrunErr );
			*dst++ = (uint8_t)( ( v >> 8 ) & 0xFF );
			*dst++ = (uint8_t)(   v        & 0xFF );
			sawDigit = 0;
			v = 0;
			continue;
		}
		
		// Handle IPv4-mapped/compatible addresses (e.g. ::FFFF:1.2.3.4).
		
		if( inAllowV4Mapped && ( c == '.' ) && ( ( dst + 4 ) <= lim ) )
		{
			err = ParseIPv4Address( ptr, dst, &inStr );
			require_noerr_quiet( err, exit );
			dst += 4;
			sawDigit = 0;
			++inStr; // Increment because the code below expects the end to be at "inStr - 1".
		}
		break;
	}
	if( sawDigit )
	{
		require_action_quiet( ( dst + 2 ) <= lim, exit, err = kOverrunErr );
		*dst++ = (uint8_t)( ( v >> 8 ) & 0xFF );
		*dst++ = (uint8_t)(   v        & 0xFF );
	}
	check( dst <= lim );
	if( colonPtr )
	{
		require_action_quiet( dst < lim, exit, err = kOverrunErr );
		n = (int)( dst - colonPtr );
		for( i = 1; i <= n; ++i )
		{
			lim[ -i ] = colonPtr[ n - i ];
			colonPtr[ n - i ] = 0;
		}
		dst = lim;
	}
	require_action_quiet( dst == lim, exit, err = kUnderrunErr );
	
	*outStr = inStr - 1;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	ParseIPv6Scope
//
//	Note: This was copied from CoreUtils because the StringToIPv6Address function is currently not exported in the framework.
//===========================================================================================================================

static OSStatus	ParseIPv6Scope( const char *inStr, uint32_t *outScope, const char **outStr )
{
#if( TARGET_OS_POSIX )
	OSStatus			err;
	char				scopeStr[ 64 ];
	char *				dst;
	char *				lim;
	int					c;
	uint32_t			scope;
	const char *		ptr;
	
	// Copy into a local NULL-terminated string since that is what if_nametoindex expects.
	
	dst = scopeStr;
	lim = dst + ( countof( scopeStr ) - 1 );
	while( ( ( c = *inStr ) != '\0' ) && ( c != ':' ) && ( c != '/' ) && ( c != ']' ) && ( dst < lim ) )
	{
		*dst++ = *inStr++;
	}
	*dst = '\0';
	check( dst <= lim );
	
	// First try to map as a name and if that fails, treat it as a numeric scope.
	
	scope = if_nametoindex( scopeStr );
	if( scope == 0 )
	{
		for( ptr = scopeStr; ( ( c = *ptr ) >= '0' ) && ( c <= '9' ); ++ptr )
		{
			scope = ( scope * 10 ) + ( ( (uint8_t) c ) - '0' );
		}
		require_action_quiet( c == '\0', exit, err = kMalformedErr );
		require_action_quiet( ( ptr != scopeStr ) && ( ( (int)( ptr - scopeStr ) ) <= 10 ), exit, err = kMalformedErr );
	}
	
	*outScope = scope;
	*outStr   = inStr;
	err = kNoErr;
	
exit:
	return( err );
#else
	OSStatus			err;
	uint32_t			scope;
	const char *		start;
	int					c;
	
	scope = 0;
	for( start = inStr; ( ( c = *inStr ) >= '0' ) && ( c <= '9' ); ++inStr )
	{
		scope = ( scope * 10 ) + ( c - '0' );
	}
	require_action_quiet( ( inStr != start ) && ( ( (int)( inStr - start ) ) <= 10 ), exit, err = kMalformedErr );
	
	*outScope = scope;
	*outStr   = inStr;
	err = kNoErr;
	
exit:
	return( err );
#endif
}

//===========================================================================================================================
//	StringToIPv6Address
//
//	Note: This was copied from CoreUtils because the StringToIPv6Address function is currently not exported in the framework.
//===========================================================================================================================

OSStatus
	StringToIPv6Address( 
		const char *			inStr, 
		StringToIPAddressFlags	inFlags, 
		uint8_t					outIPv6[ 16 ], 
		uint32_t *				outScope, 
		int *					outPort, 
		int *					outPrefix, 
		const char **			outStr )
{
	OSStatus		err;
	uint8_t			ipv6[ 16 ];
	int				c;
	int				hasScope;
	uint32_t		scope;
	int				hasPort;
	int				port;
	int				hasPrefix;
	int				prefix;
	int				hasBracket;
	int				i;
	
	require_action( inStr, exit, err = kParamErr );
	
	if( *inStr == '[' ) ++inStr; // Skip a leading bracket for []-wrapped addresses (e.g. "[::1]:80").
	
	// Parse the address-only part of the address (e.g. "1::1").
	
	err = ParseIPv6Address( inStr, !( inFlags & kStringToIPAddressFlagsNoIPv4Mapped ), ipv6, &inStr );
	require_noerr_quiet( err, exit );
	c = *inStr;
	
	// Parse the scope, port, or prefix length.
	
	hasScope	= 0;
	scope		= 0;
	hasPort		= 0;
	port		= 0;
	hasPrefix	= 0;
	prefix		= 0;
	hasBracket	= 0;
	for( ;; )
	{
		if( c == '%' )		// Scope (e.g. "%en0" or "%5")
		{
			require_action_quiet( !hasScope, exit, err = kMalformedErr );
			require_action_quiet( !( inFlags & kStringToIPAddressFlagsNoScope ), exit, err = kUnexpectedErr );
			++inStr;
			err = ParseIPv6Scope( inStr, &scope, &inStr );
			require_noerr_quiet( err, exit );
			hasScope = 1;
			c = *inStr;
		}
		else if( c == ':' )	// Port (e.g. ":80")
		{
			require_action_quiet( !hasPort, exit, err = kMalformedErr );
			require_action_quiet( !( inFlags & kStringToIPAddressFlagsNoPort ), exit, err = kUnexpectedErr );
			while( ( ( c = *( ++inStr ) ) != '\0' ) && ( ( c >= '0' ) && ( c <= '9' ) ) ) port = ( port * 10 ) + ( c - '0' );
			require_action_quiet( port <= 65535, exit, err = kRangeErr );
			hasPort = 1;
		}
		else if( c == '/' )	// Prefix Length (e.g. "/64")
		{
			require_action_quiet( !hasPrefix, exit, err = kMalformedErr );
			require_action_quiet( !( inFlags & kStringToIPAddressFlagsNoPrefix ), exit, err = kUnexpectedErr );
			while( ( ( c = *( ++inStr ) ) != '\0' ) && ( ( c >= '0' ) && ( c <= '9' ) ) ) prefix = ( prefix * 10 ) + ( c - '0' );
			require_action_quiet( ( prefix >= 0 ) && ( prefix <= 128 ), exit, err = kRangeErr );
			hasPrefix = 1;
		}
		else if( c == ']' )
		{
			require_action_quiet( !hasBracket, exit, err = kMalformedErr );
			hasBracket = 1;
			c = *( ++inStr );
		}
		else
		{
			break;
		}
	}
	
	// Return the results. Only fill in scope/port/prefix results if the info was found to allow for defaults.
	
	if( outIPv6 )				 for( i = 0; i < 16; ++i ) outIPv6[ i ] = ipv6[ i ];
	if( outScope  && hasScope )  *outScope	= scope;
	if( outPort   && hasPort )   *outPort	= port;
	if( outPrefix && hasPrefix ) *outPrefix	= prefix;
	if( outStr )				 *outStr	= inStr;
	err = kNoErr;
	
exit:
	return( err );
}

//===========================================================================================================================
//	StringArray_Append
//
//	Note: This was copied from CoreUtils because the StringArray_Append function is currently not exported in the framework.
//===========================================================================================================================

OSStatus	StringArray_Append( char ***ioArray, size_t *ioCount, const char *inStr )
{
	OSStatus		err;
	char *			newStr;
	size_t			oldCount;
	size_t			newCount;
	char **			oldArray;
	char **			newArray;
	
	newStr = strdup( inStr );
	require_action( newStr, exit, err = kNoMemoryErr );
	
	oldCount = *ioCount;
	newCount = oldCount + 1;
	newArray = (char **) malloc( newCount * sizeof( *newArray ) );
	require_action( newArray, exit, err = kNoMemoryErr );
	
	if( oldCount > 0 )
	{
		oldArray = *ioArray;
		memcpy( newArray, oldArray, oldCount * sizeof( *oldArray ) );
		free( oldArray );
	}
	newArray[ oldCount ] = newStr;
	newStr = NULL;
	
	*ioArray = newArray;
	*ioCount = newCount;
	err = kNoErr;
	
exit:
	if( newStr ) free( newStr );
	return( err );
}

//===========================================================================================================================
//	StringArray_Free
//
//	Note: This was copied from CoreUtils because the StringArray_Free function is currently not exported in the framework.
//===========================================================================================================================

void	StringArray_Free( char **inArray, size_t inCount )
{
	size_t		i;
	
	for( i = 0; i < inCount; ++i )
	{
		free( inArray[ i ] );
	}
	if( inCount > 0 ) free( inArray );
}

//===========================================================================================================================
//	ParseQuotedEscapedString
//
//	Note: This was copied from CoreUtils because it's currently not exported in the framework.
//===========================================================================================================================

Boolean
	ParseQuotedEscapedString( 
		const char *	inSrc, 
		const char *	inEnd, 
		const char *	inDelimiters, 
		char *			inBuf, 
		size_t			inMaxLen, 
		size_t *		outCopiedLen, 
		size_t *		outTotalLen, 
		const char **	outSrc )
{
	const unsigned char *		src;
	const unsigned char *		end;
	unsigned char *				dst;
	unsigned char *				lim;
	unsigned char				c;
	unsigned char				c2;
	size_t						totalLen;
	Boolean						singleQuote;
	Boolean						doubleQuote;
	
	if( inEnd == NULL ) inEnd = inSrc + strlen( inSrc );
	src = (const unsigned char *) inSrc;
	end = (const unsigned char *) inEnd;
	dst = (unsigned char *) inBuf;
	lim = dst + inMaxLen;
	while( ( src < end ) && isspace_safe( *src ) ) ++src; // Skip leading spaces.
	if( src >= end ) return( false );
	
	// Parse each argument from the string.
	//
	// See <http://resources.mpi-inf.mpg.de/departments/rg1/teaching/unixffb-ss98/quoting-guide.html> for details.
	
	totalLen = 0;
	singleQuote = false;
	doubleQuote = false;
	while( src < end )
	{
		c = *src++;
		if( singleQuote )
		{
			// Single quotes protect everything (even backslashes, newlines, etc.) except single quotes.
			
			if( c == '\'' )
			{
				singleQuote = false;
				continue;
			}
		}
		else if( doubleQuote )
		{
			// Double quotes protect everything except double quotes and backslashes. A backslash can be 
			// used to protect " or \ within double quotes. A backslash-newline pair disappears completely.
			// A backslash followed by x or X and 2 hex digits (e.g. "\x1f") is stored as that hex byte.
			// A backslash followed by 3 octal digits (e.g. "\377") is stored as that octal byte.
			// A backslash that does not precede ", \, x, X, or a newline is taken literally.
			
			if( c == '"' )
			{
				doubleQuote = false;
				continue;
			}
			else if( c == '\\' )
			{
				if( src < end )
				{
					c2 = *src;
					if( ( c2 == '"' ) || ( c2 == '\\' ) )
					{
						++src;
						c = c2;
					}
					else if( c2 == '\n' )
					{
						++src;
						continue;
					}
					else if( ( c2 == 'x' ) || ( c2 == 'X' ) )
					{
						++src;
						c = c2;
						if( ( ( end - src ) >= 2 ) && IsHexPair( src ) )
						{
							c = HexPairToByte( src );
							src += 2;
						}
					}
					else if( isoctal_safe( c2 ) )
					{
						if( ( ( end - src ) >= 3 ) && IsOctalTriple( src ) )
						{
							c = OctalTripleToByte( src );
							src += 3;
						}
					}
				}
			}
		}
		else if( strchr( inDelimiters, c ) )
		{
			break;
		}
		else if( c == '\\' )
		{
			// A backslash protects the next character, except a newline, x, X and 2 hex bytes or 3 octal bytes. 
			// A backslash followed by a newline disappears completely.
			// A backslash followed by x or X and 2 hex digits (e.g. "\x1f") is stored as that hex byte.
			// A backslash followed by 3 octal digits (e.g. "\377") is stored as that octal byte.
			
			if( src < end )
			{
				c = *src;
				if( c == '\n' )
				{
					++src;
					continue;
				}
				else if( ( c == 'x' ) || ( c == 'X' ) )
				{
					++src;
					if( ( ( end - src ) >= 2 ) && IsHexPair( src ) )
					{
						c = HexPairToByte( src );
						src += 2;
					}
				}
				else if( isoctal_safe( c ) )
				{
					if( ( ( end - src ) >= 3 ) && IsOctalTriple( src ) )
					{
						c = OctalTripleToByte( src );
						src += 3;
					}
					else
					{
						++src;
					}
				}
				else
				{
					++src;
				}
			}
		}
		else if( c == '\'' )
		{
			singleQuote = true;
			continue;
		}
		else if( c == '"' )
		{
			doubleQuote = true;
			continue;
		}
		
		if( dst < lim )
		{
			if( inBuf ) *dst = c;
			++dst;
		}
		++totalLen;
	}
	
	if( outCopiedLen )	*outCopiedLen	= (size_t)( dst - ( (unsigned char *) inBuf ) );
	if( outTotalLen )	*outTotalLen	= totalLen;
	if( outSrc )		*outSrc			= (const char *) src;
	return( true );
}
