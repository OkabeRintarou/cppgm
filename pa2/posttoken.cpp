// (C) 2013 CPPGM Foundation www.cppgm.org.  All rights reserved.

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <memory>
#include <cstring>
#include <cstdint>
#include <climits>
#include <map>

#include "DebugPPTokenStream.h"

using namespace std;

// See 3.9.1: Fundamental Types
enum EFundamentalType {
  // 3.9.1.2
    FT_SIGNED_CHAR,
  FT_SHORT_INT,
  FT_INT,
  FT_LONG_INT,
  FT_LONG_LONG_INT,

  // 3.9.1.3
    FT_UNSIGNED_CHAR,
  FT_UNSIGNED_SHORT_INT,
  FT_UNSIGNED_INT,
  FT_UNSIGNED_LONG_INT,
  FT_UNSIGNED_LONG_LONG_INT,

  // 3.9.1.1 / 3.9.1.5
    FT_WCHAR_T,
  FT_CHAR,
  FT_CHAR16_T,
  FT_CHAR32_T,

  // 3.9.1.6
    FT_BOOL,

  // 3.9.1.8
    FT_FLOAT,
  FT_DOUBLE,
  FT_LONG_DOUBLE,

  // 3.9.1.9
    FT_VOID,

  // 3.9.1.10
    FT_NULLPTR_T
};

// FundamentalTypeOf: convert fundamental type T to EFundamentalType
// for example: `FundamentalTypeOf<long int>()` will return `FT_LONG_INT`
template<typename T>
constexpr EFundamentalType FundamentalTypeOf();

template<>
constexpr EFundamentalType FundamentalTypeOf<signed char>() { return FT_SIGNED_CHAR; }

template<>
constexpr EFundamentalType FundamentalTypeOf<short int>() { return FT_SHORT_INT; }

template<>
constexpr EFundamentalType FundamentalTypeOf<int>() { return FT_INT; }

template<>
constexpr EFundamentalType FundamentalTypeOf<long int>() { return FT_LONG_INT; }

template<>
constexpr EFundamentalType FundamentalTypeOf<long long int>() { return FT_LONG_LONG_INT; }

template<>
constexpr EFundamentalType FundamentalTypeOf<unsigned char>() { return FT_UNSIGNED_CHAR; }

template<>
constexpr EFundamentalType FundamentalTypeOf<unsigned short int>() { return FT_UNSIGNED_SHORT_INT; }

template<>
constexpr EFundamentalType FundamentalTypeOf<unsigned int>() { return FT_UNSIGNED_INT; }

template<>
constexpr EFundamentalType FundamentalTypeOf<unsigned long int>() { return FT_UNSIGNED_LONG_INT; }

template<>
constexpr EFundamentalType FundamentalTypeOf<unsigned long long int>() { return FT_UNSIGNED_LONG_LONG_INT; }

template<>
constexpr EFundamentalType FundamentalTypeOf<wchar_t>() { return FT_WCHAR_T; }

template<>
constexpr EFundamentalType FundamentalTypeOf<char>() { return FT_CHAR; }

template<>
constexpr EFundamentalType FundamentalTypeOf<char16_t>() { return FT_CHAR16_T; }

template<>
constexpr EFundamentalType FundamentalTypeOf<char32_t>() { return FT_CHAR32_T; }

template<>
constexpr EFundamentalType FundamentalTypeOf<bool>() { return FT_BOOL; }

template<>
constexpr EFundamentalType FundamentalTypeOf<float>() { return FT_FLOAT; }

template<>
constexpr EFundamentalType FundamentalTypeOf<double>() { return FT_DOUBLE; }

template<>
constexpr EFundamentalType FundamentalTypeOf<long double>() { return FT_LONG_DOUBLE; }

template<>
constexpr EFundamentalType FundamentalTypeOf<void>() { return FT_VOID; }

template<>
constexpr EFundamentalType FundamentalTypeOf<nullptr_t>() { return FT_NULLPTR_T; }

// convert EFundamentalType to a source code
const map<EFundamentalType, string> FundamentalTypeToStringMap
  {
    {FT_SIGNED_CHAR,            "signed char"},
    {FT_SHORT_INT,              "short int"},
    {FT_INT,                    "int"},
    {FT_LONG_INT,               "long int"},
    {FT_LONG_LONG_INT,          "long long int"},
    {FT_UNSIGNED_CHAR,          "unsigned char"},
    {FT_UNSIGNED_SHORT_INT,     "unsigned short int"},
    {FT_UNSIGNED_INT,           "unsigned int"},
    {FT_UNSIGNED_LONG_INT,      "unsigned long int"},
    {FT_UNSIGNED_LONG_LONG_INT, "unsigned long long int"},
    {FT_WCHAR_T,                "wchar_t"},
    {FT_CHAR,                   "char"},
    {FT_CHAR16_T,               "char16_t"},
    {FT_CHAR32_T,               "char32_t"},
    {FT_BOOL,                   "bool"},
    {FT_FLOAT,                  "float"},
    {FT_DOUBLE,                 "double"},
    {FT_LONG_DOUBLE,            "long double"},
    {FT_VOID,                   "void"},
    {FT_NULLPTR_T,              "nullptr_t"}
  };

// token type enum for `simples`
enum ETokenType {
  // keywords
    KW_ALIGNAS,
  KW_ALIGNOF,
  KW_ASM,
  KW_AUTO,
  KW_BOOL,
  KW_BREAK,
  KW_CASE,
  KW_CATCH,
  KW_CHAR,
  KW_CHAR16_T,
  KW_CHAR32_T,
  KW_CLASS,
  KW_CONST,
  KW_CONSTEXPR,
  KW_CONST_CAST,
  KW_CONTINUE,
  KW_DECLTYPE,
  KW_DEFAULT,
  KW_DELETE,
  KW_DO,
  KW_DOUBLE,
  KW_DYNAMIC_CAST,
  KW_ELSE,
  KW_ENUM,
  KW_EXPLICIT,
  KW_EXPORT,
  KW_EXTERN,
  KW_FALSE,
  KW_FLOAT,
  KW_FOR,
  KW_FRIEND,
  KW_GOTO,
  KW_IF,
  KW_INLINE,
  KW_INT,
  KW_LONG,
  KW_MUTABLE,
  KW_NAMESPACE,
  KW_NEW,
  KW_NOEXCEPT,
  KW_NULLPTR,
  KW_OPERATOR,
  KW_PRIVATE,
  KW_PROTECTED,
  KW_PUBLIC,
  KW_REGISTER,
  KW_REINTERPET_CAST,
  KW_RETURN,
  KW_SHORT,
  KW_SIGNED,
  KW_SIZEOF,
  KW_STATIC,
  KW_STATIC_ASSERT,
  KW_STATIC_CAST,
  KW_STRUCT,
  KW_SWITCH,
  KW_TEMPLATE,
  KW_THIS,
  KW_THREAD_LOCAL,
  KW_THROW,
  KW_TRUE,
  KW_TRY,
  KW_TYPEDEF,
  KW_TYPEID,
  KW_TYPENAME,
  KW_UNION,
  KW_UNSIGNED,
  KW_USING,
  KW_VIRTUAL,
  KW_VOID,
  KW_VOLATILE,
  KW_WCHAR_T,
  KW_WHILE,

  // operators/punctuation
    OP_LBRACE,
  OP_RBRACE,
  OP_LSQUARE,
  OP_RSQUARE,
  OP_LPAREN,
  OP_RPAREN,
  OP_BOR,
  OP_XOR,
  OP_COMPL,
  OP_AMP,
  OP_LNOT,
  OP_SEMICOLON,
  OP_COLON,
  OP_DOTS,
  OP_QMARK,
  OP_COLON2,
  OP_DOT,
  OP_DOTSTAR,
  OP_PLUS,
  OP_MINUS,
  OP_STAR,
  OP_DIV,
  OP_MOD,
  OP_ASS,
  OP_LT,
  OP_GT,
  OP_PLUSASS,
  OP_MINUSASS,
  OP_STARASS,
  OP_DIVASS,
  OP_MODASS,
  OP_XORASS,
  OP_BANDASS,
  OP_BORASS,
  OP_LSHIFT,
  OP_RSHIFT,
  OP_RSHIFTASS,
  OP_LSHIFTASS,
  OP_EQ,
  OP_NE,
  OP_LE,
  OP_GE,
  OP_LAND,
  OP_LOR,
  OP_INC,
  OP_DEC,
  OP_COMMA,
  OP_ARROWSTAR,
  OP_ARROW,
};

// StringToETokenTypeMap map of `simple` `preprocessing-tokens` to ETokenType
const unordered_map<string, ETokenType> StringToTokenTypeMap =
  {
    // keywords
    {"alignas",          KW_ALIGNAS},
    {"alignof",          KW_ALIGNOF},
    {"asm",              KW_ASM},
    {"auto",             KW_AUTO},
    {"bool",             KW_BOOL},
    {"break",            KW_BREAK},
    {"case",             KW_CASE},
    {"catch",            KW_CATCH},
    {"char",             KW_CHAR},
    {"char16_t",         KW_CHAR16_T},
    {"char32_t",         KW_CHAR32_T},
    {"class",            KW_CLASS},
    {"const",            KW_CONST},
    {"constexpr",        KW_CONSTEXPR},
    {"const_cast",       KW_CONST_CAST},
    {"continue",         KW_CONTINUE},
    {"decltype",         KW_DECLTYPE},
    {"default",          KW_DEFAULT},
    {"delete",           KW_DELETE},
    {"do",               KW_DO},
    {"double",           KW_DOUBLE},
    {"dynamic_cast",     KW_DYNAMIC_CAST},
    {"else",             KW_ELSE},
    {"enum",             KW_ENUM},
    {"explicit",         KW_EXPLICIT},
    {"export",           KW_EXPORT},
    {"extern",           KW_EXTERN},
    {"false",            KW_FALSE},
    {"float",            KW_FLOAT},
    {"for",              KW_FOR},
    {"friend",           KW_FRIEND},
    {"goto",             KW_GOTO},
    {"if",               KW_IF},
    {"inline",           KW_INLINE},
    {"int",              KW_INT},
    {"long",             KW_LONG},
    {"mutable",          KW_MUTABLE},
    {"namespace",        KW_NAMESPACE},
    {"new",              KW_NEW},
    {"noexcept",         KW_NOEXCEPT},
    {"nullptr",          KW_NULLPTR},
    {"operator",         KW_OPERATOR},
    {"private",          KW_PRIVATE},
    {"protected",        KW_PROTECTED},
    {"public",           KW_PUBLIC},
    {"register",         KW_REGISTER},
    {"reinterpret_cast", KW_REINTERPET_CAST},
    {"return",           KW_RETURN},
    {"short",            KW_SHORT},
    {"signed",           KW_SIGNED},
    {"sizeof",           KW_SIZEOF},
    {"static",           KW_STATIC},
    {"static_assert",    KW_STATIC_ASSERT},
    {"static_cast",      KW_STATIC_CAST},
    {"struct",           KW_STRUCT},
    {"switch",           KW_SWITCH},
    {"template",         KW_TEMPLATE},
    {"this",             KW_THIS},
    {"thread_local",     KW_THREAD_LOCAL},
    {"throw",            KW_THROW},
    {"true",             KW_TRUE},
    {"try",              KW_TRY},
    {"typedef",          KW_TYPEDEF},
    {"typeid",           KW_TYPEID},
    {"typename",         KW_TYPENAME},
    {"union",            KW_UNION},
    {"unsigned",         KW_UNSIGNED},
    {"using",            KW_USING},
    {"virtual",          KW_VIRTUAL},
    {"void",             KW_VOID},
    {"volatile",         KW_VOLATILE},
    {"wchar_t",          KW_WCHAR_T},
    {"while",            KW_WHILE},

    // operators/punctuation
    {"{",                OP_LBRACE},
    {"<%",               OP_LBRACE},
    {"}",                OP_RBRACE},
    {"%>",               OP_RBRACE},
    {"[",                OP_LSQUARE},
    {"<:",               OP_LSQUARE},
    {"]",                OP_RSQUARE},
    {":>",               OP_RSQUARE},
    {"(",                OP_LPAREN},
    {")",                OP_RPAREN},
    {"|",                OP_BOR},
    {"bitor",            OP_BOR},
    {"^",                OP_XOR},
    {"xor",              OP_XOR},
    {"~",                OP_COMPL},
    {"compl",            OP_COMPL},
    {"&",                OP_AMP},
    {"bitand",           OP_AMP},
    {"!",                OP_LNOT},
    {"not",              OP_LNOT},
    {";",                OP_SEMICOLON},
    {":",                OP_COLON},
    {"...",              OP_DOTS},
    {"?",                OP_QMARK},
    {"::",               OP_COLON2},
    {".",                OP_DOT},
    {".*",               OP_DOTSTAR},
    {"+",                OP_PLUS},
    {"-",                OP_MINUS},
    {"*",                OP_STAR},
    {"/",                OP_DIV},
    {"%",                OP_MOD},
    {"=",                OP_ASS},
    {"<",                OP_LT},
    {">",                OP_GT},
    {"+=",               OP_PLUSASS},
    {"-=",               OP_MINUSASS},
    {"*=",               OP_STARASS},
    {"/=",               OP_DIVASS},
    {"%=",               OP_MODASS},
    {"^=",               OP_XORASS},
    {"xor_eq",           OP_XORASS},
    {"&=",               OP_BANDASS},
    {"and_eq",           OP_BANDASS},
    {"|=",               OP_BORASS},
    {"or_eq",            OP_BORASS},
    {"<<",               OP_LSHIFT},
    {">>",               OP_RSHIFT},
    {">>=",              OP_RSHIFTASS},
    {"<<=",              OP_LSHIFTASS},
    {"==",               OP_EQ},
    {"!=",               OP_NE},
    {"not_eq",           OP_NE},
    {"<=",               OP_LE},
    {">=",               OP_GE},
    {"&&",               OP_LAND},
    {"and",              OP_LAND},
    {"||",               OP_LOR},
    {"or",               OP_LOR},
    {"++",               OP_INC},
    {"--",               OP_DEC},
    {",",                OP_COMMA},
    {"->*",              OP_ARROWSTAR},
    {"->",               OP_ARROW}
  };

// map of enum to string
const map<ETokenType, string> TokenTypeToStringMap =
  {
    {KW_ALIGNAS,         "KW_ALIGNAS"},
    {KW_ALIGNOF,         "KW_ALIGNOF"},
    {KW_ASM,             "KW_ASM"},
    {KW_AUTO,            "KW_AUTO"},
    {KW_BOOL,            "KW_BOOL"},
    {KW_BREAK,           "KW_BREAK"},
    {KW_CASE,            "KW_CASE"},
    {KW_CATCH,           "KW_CATCH"},
    {KW_CHAR,            "KW_CHAR"},
    {KW_CHAR16_T,        "KW_CHAR16_T"},
    {KW_CHAR32_T,        "KW_CHAR32_T"},
    {KW_CLASS,           "KW_CLASS"},
    {KW_CONST,           "KW_CONST"},
    {KW_CONSTEXPR,       "KW_CONSTEXPR"},
    {KW_CONST_CAST,      "KW_CONST_CAST"},
    {KW_CONTINUE,        "KW_CONTINUE"},
    {KW_DECLTYPE,        "KW_DECLTYPE"},
    {KW_DEFAULT,         "KW_DEFAULT"},
    {KW_DELETE,          "KW_DELETE"},
    {KW_DO,              "KW_DO"},
    {KW_DOUBLE,          "KW_DOUBLE"},
    {KW_DYNAMIC_CAST,    "KW_DYNAMIC_CAST"},
    {KW_ELSE,            "KW_ELSE"},
    {KW_ENUM,            "KW_ENUM"},
    {KW_EXPLICIT,        "KW_EXPLICIT"},
    {KW_EXPORT,          "KW_EXPORT"},
    {KW_EXTERN,          "KW_EXTERN"},
    {KW_FALSE,           "KW_FALSE"},
    {KW_FLOAT,           "KW_FLOAT"},
    {KW_FOR,             "KW_FOR"},
    {KW_FRIEND,          "KW_FRIEND"},
    {KW_GOTO,            "KW_GOTO"},
    {KW_IF,              "KW_IF"},
    {KW_INLINE,          "KW_INLINE"},
    {KW_INT,             "KW_INT"},
    {KW_LONG,            "KW_LONG"},
    {KW_MUTABLE,         "KW_MUTABLE"},
    {KW_NAMESPACE,       "KW_NAMESPACE"},
    {KW_NEW,             "KW_NEW"},
    {KW_NOEXCEPT,        "KW_NOEXCEPT"},
    {KW_NULLPTR,         "KW_NULLPTR"},
    {KW_OPERATOR,        "KW_OPERATOR"},
    {KW_PRIVATE,         "KW_PRIVATE"},
    {KW_PROTECTED,       "KW_PROTECTED"},
    {KW_PUBLIC,          "KW_PUBLIC"},
    {KW_REGISTER,        "KW_REGISTER"},
    {KW_REINTERPET_CAST, "KW_REINTERPET_CAST"},
    {KW_RETURN,          "KW_RETURN"},
    {KW_SHORT,           "KW_SHORT"},
    {KW_SIGNED,          "KW_SIGNED"},
    {KW_SIZEOF,          "KW_SIZEOF"},
    {KW_STATIC,          "KW_STATIC"},
    {KW_STATIC_ASSERT,   "KW_STATIC_ASSERT"},
    {KW_STATIC_CAST,     "KW_STATIC_CAST"},
    {KW_STRUCT,          "KW_STRUCT"},
    {KW_SWITCH,          "KW_SWITCH"},
    {KW_TEMPLATE,        "KW_TEMPLATE"},
    {KW_THIS,            "KW_THIS"},
    {KW_THREAD_LOCAL,    "KW_THREAD_LOCAL"},
    {KW_THROW,           "KW_THROW"},
    {KW_TRUE,            "KW_TRUE"},
    {KW_TRY,             "KW_TRY"},
    {KW_TYPEDEF,         "KW_TYPEDEF"},
    {KW_TYPEID,          "KW_TYPEID"},
    {KW_TYPENAME,        "KW_TYPENAME"},
    {KW_UNION,           "KW_UNION"},
    {KW_UNSIGNED,        "KW_UNSIGNED"},
    {KW_USING,           "KW_USING"},
    {KW_VIRTUAL,         "KW_VIRTUAL"},
    {KW_VOID,            "KW_VOID"},
    {KW_VOLATILE,        "KW_VOLATILE"},
    {KW_WCHAR_T,         "KW_WCHAR_T"},
    {KW_WHILE,           "KW_WHILE"},
    {OP_LBRACE,          "OP_LBRACE"},
    {OP_RBRACE,          "OP_RBRACE"},
    {OP_LSQUARE,         "OP_LSQUARE"},
    {OP_RSQUARE,         "OP_RSQUARE"},
    {OP_LPAREN,          "OP_LPAREN"},
    {OP_RPAREN,          "OP_RPAREN"},
    {OP_BOR,             "OP_BOR"},
    {OP_XOR,             "OP_XOR"},
    {OP_COMPL,           "OP_COMPL"},
    {OP_AMP,             "OP_AMP"},
    {OP_LNOT,            "OP_LNOT"},
    {OP_SEMICOLON,       "OP_SEMICOLON"},
    {OP_COLON,           "OP_COLON"},
    {OP_DOTS,            "OP_DOTS"},
    {OP_QMARK,           "OP_QMARK"},
    {OP_COLON2,          "OP_COLON2"},
    {OP_DOT,             "OP_DOT"},
    {OP_DOTSTAR,         "OP_DOTSTAR"},
    {OP_PLUS,            "OP_PLUS"},
    {OP_MINUS,           "OP_MINUS"},
    {OP_STAR,            "OP_STAR"},
    {OP_DIV,             "OP_DIV"},
    {OP_MOD,             "OP_MOD"},
    {OP_ASS,             "OP_ASS"},
    {OP_LT,              "OP_LT"},
    {OP_GT,              "OP_GT"},
    {OP_PLUSASS,         "OP_PLUSASS"},
    {OP_MINUSASS,        "OP_MINUSASS"},
    {OP_STARASS,         "OP_STARASS"},
    {OP_DIVASS,          "OP_DIVASS"},
    {OP_MODASS,          "OP_MODASS"},
    {OP_XORASS,          "OP_XORASS"},
    {OP_BANDASS,         "OP_BANDASS"},
    {OP_BORASS,          "OP_BORASS"},
    {OP_LSHIFT,          "OP_LSHIFT"},
    {OP_RSHIFT,          "OP_RSHIFT"},
    {OP_RSHIFTASS,       "OP_RSHIFTASS"},
    {OP_LSHIFTASS,       "OP_LSHIFTASS"},
    {OP_EQ,              "OP_EQ"},
    {OP_NE,              "OP_NE"},
    {OP_LE,              "OP_LE"},
    {OP_GE,              "OP_GE"},
    {OP_LAND,            "OP_LAND"},
    {OP_LOR,             "OP_LOR"},
    {OP_INC,             "OP_INC"},
    {OP_DEC,             "OP_DEC"},
    {OP_COMMA,           "OP_COMMA"},
    {OP_ARROWSTAR,       "OP_ARROWSTAR"},
    {OP_ARROW,           "OP_ARROW"}
  };

// convert integer [0,15] to hexadecimal digit
char ValueToHexChar(int c) {
  switch (c) {
    case 0:
      return '0';
    case 1:
      return '1';
    case 2:
      return '2';
    case 3:
      return '3';
    case 4:
      return '4';
    case 5:
      return '5';
    case 6:
      return '6';
    case 7:
      return '7';
    case 8:
      return '8';
    case 9:
      return '9';
    case 10:
      return 'A';
    case 11:
      return 'B';
    case 12:
      return 'C';
    case 13:
      return 'D';
    case 14:
      return 'E';
    case 15:
      return 'F';
    default:
      throw logic_error("ValueToHexChar of nonhex value");
  }
}

// hex dump memory range
string HexDump(const void *pdata, size_t nbytes) {
  unsigned char *p = (unsigned char *) pdata;

  string s(nbytes *
           2, '?');

  for (size_t i = 0; i < nbytes; i++) {
    s[2 * i + 0] = ValueToHexChar((p[i] & 0xF0) >> 4);
    s[2 * i + 1] = ValueToHexChar((p[i] & 0x0F) >> 0);
  }

  return s;
}

// DebugPostTokenOutputStream: helper class to produce PA2 output format
struct DebugPostTokenOutputStream {
  // output: invalid <source>
  void emit_invalid(const string &source) {
    cout << "invalid " << source << endl;
  }

  // output: simple <source> <token_type>
  void emit_simple(const string &source, ETokenType token_type) {
    cout << "simple " << source << " " << TokenTypeToStringMap.at(token_type) << endl;
  }

  // output: identifier <source>
  void emit_identifier(const string &source) {
    cout << "identifier " << source << endl;
  }

  // output: literal <source> <type> <hexdump(data,nbytes)>
  void emit_literal(const string &source, EFundamentalType type, const void *data, size_t nbytes) {
    cout << "literal " << source << " " << FundamentalTypeToStringMap.at(type) << " " << HexDump(data, nbytes)
         << endl;
  }

  // output: literal <source> array of <num_elements> <type> <hexdump(data,nbytes)>
  void emit_literal_array(const string &source, size_t num_elements, EFundamentalType type, const void *data,
                          size_t nbytes) {
    cout << "literal " << source << " array of " << num_elements << " " << FundamentalTypeToStringMap.at(type) << " "
         << HexDump(data, nbytes) << endl;
  }

  // output: user-defined-literal <source> <ud_suffix> character <type> <hexdump(data,nbytes)>
  void emit_user_defined_literal_character(const string &source, const string &ud_suffix, EFundamentalType type,
                                           const void *data, size_t nbytes) {
    cout << "user-defined-literal " << source << " " << ud_suffix << " character "
         << FundamentalTypeToStringMap.at(type) << " " << HexDump(data, nbytes) << endl;
  }

  // output: user-defined-literal <source> <ud_suffix> string array of <num_elements> <type> <hexdump(data, nbytes)>
  void emit_user_defined_literal_string_array(const string &source, const string &ud_suffix, size_t num_elements,
                                              EFundamentalType type, const void *data, size_t nbytes) {
    cout << "user-defined-literal " << source << " " << ud_suffix << " string array of " << num_elements << " "
         << FundamentalTypeToStringMap.at(type) << " " << HexDump(data, nbytes) << endl;
  }

  // output: user-defined-literal <source> <ud_suffix> <prefix>
  void emit_user_defined_literal_integer(const string &source, const string &ud_suffix, const string &prefix) {
    cout << "user-defined-literal " << source << " " << ud_suffix << " integer " << prefix << endl;
  }

  // output: user-defined-literal <source> <ud_suffix> <prefix>
  void emit_user_defined_literal_floating(const string &source, const string &ud_suffix, const string &prefix) {
    cout << "user-defined-literal " << source << " " << ud_suffix << " floating " << prefix << endl;
  }

  // output : eof
  void emit_eof() {
    cout << "eof" << endl;
  }
};


// use these 3 functions to scan `floating-literals` (see PA2)
// for example PA2Decode_float("12.34") returns "12.34" as a `float` type
float PA2Decode_float(const string &s) {
  istringstream iss(s);
  float x;
  iss >> x;
  return x;
}

double PA2Decode_double(const string &s) {
  istringstream iss(s);
  double x;
  iss >> x;
  return x;
}

long double PA2Decode_long_double(const string &s) {
  istringstream iss(s);
  long double x;
  iss >> x;
  return x;
}

static inline bool isInvalidOperator(const string &data) {
  return data == "%:%:" || data == "%:" || data == "##" || data == "#";
}

static inline bool isHex(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

struct PostTokenizer {


  PostTokenizer(DebugPostTokenOutputStream &out) : output(out) {}

  void process(const PPToken &token) {

    switch (token.type) {
      case PPTokenType::Tk_OpOrPunc:
        process_OpOrPunc(token);
        break;
      case PPTokenType::Tk_Identifier:
        process_Identifier(token);
        break;
      case PPTokenType::Tk_PPNumber:
        process_PPNumber(token);
        break;
      case PPTokenType::Tk_HeaderName:
        process_HeaderName(token);
        break;
      case PPTokenType::Tk_EOF:
        output.emit_eof();
        break;
      default:
        ASSERT(false, "never reach here");
    }
  }

private:
  void process_OpOrPunc(const PPToken &token) {
    if (isInvalidOperator(token.data)) {
      output.emit_invalid(token.data);
    } else {
      auto it = StringToTokenTypeMap.find(token.data);
      if (it == StringToTokenTypeMap.end()) {
        output.emit_invalid(token.data);
      } else {
        output.emit_simple(token.data, it->second);
      }
    }
  }

  void process_Identifier(const PPToken &token) {
    auto it = StringToTokenTypeMap.find(token.data);
    if (it != StringToTokenTypeMap.end()) {
      output.emit_simple(token.data, it->second);
    } else {
      output.emit_identifier(token.data);
    }
  }

  EFundamentalType parseIntegerType(const string &suffix, size_t &width) {
    EFundamentalType type = FT_INT;
    width = sizeof(int);

    if (!suffix.empty()) {

      switch (suffix.size()) {
        case 1: {
          if (std::tolower(suffix[0]) == 'u') {
            type = FT_UNSIGNED_INT;
            width = sizeof(unsigned int);
          } else if (std::tolower(suffix[0]) == 'l') {
            type = FT_LONG_INT;
            width = sizeof(long int);
          } else {
            ASSERT(false, "never reach here");
          }
        }
          break;
        case 2: {
          string str(suffix);
          for (auto &c : str) {
            c = static_cast<char>((std::tolower(c)));
          }

          if (str == "ll") {
            type = FT_LONG_LONG_INT;
            width = sizeof(long long int);
          } else {
            ASSERT(str == "lu" || str == "ul", "type must be unsigned long");
            type = FT_UNSIGNED_LONG_INT;
            width = sizeof(unsigned long int);
          }
        }
          break;
        case 3: {

          type = FT_UNSIGNED_LONG_LONG_INT;
          width = sizeof(unsigned long long int);
        }

          break;
        default:
          ASSERT(false, "never reach here");
      }
    }
    return type;
  }

  EFundamentalType parseFloatType(const string &suffix, size_t &width) {
    EFundamentalType type = FT_DOUBLE;
    width = sizeof(double);

    if (!suffix.empty()) {
      switch (suffix[0]) {
        case 'f':
        case 'F':
          type = FT_FLOAT;
          width = sizeof(float);
          break;
        case 'l':
        case 'L':
          type = FT_LONG_DOUBLE;
          width = sizeof(long double);
          break;
        default:
          ASSERT(false, "never reach here");
      }
    }
    return type;
  }

  void process_PPNumber(const PPToken &token) {
    bool valid, isFloat;

    string suffix, ud_suffix;
    string::size_type index;

    valid = checkNumberLiteral(token.data, isFloat, index, suffix, ud_suffix);
    if (!valid) {
      output.emit_invalid(token.data);
      return;
    }

    bool isUser = !ud_suffix.empty();

    EFundamentalType type;
    size_t width;

    string sub = token.data.substr(0, index);
    if (isFloat) {
      type = parseFloatType(suffix, width);
    } else {
      type = parseIntegerType(suffix, width);

    }

  }

  bool checkNumberLiteral(const string &str,
                          bool &isFloat,
                          string::size_type &index,
                          string &suffix, string &ud_suffix) {

    if (str.empty()) {
      return false;
    }

    isFloat = false;
    auto sz = str.size();

    if (sz >= 2 &&
        str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
      return checkHexInteger(str, index, suffix, ud_suffix);
    } else if (str[0] == '0') {
      return checkOctInteger(str, index, suffix, ud_suffix);
    }

    index = 0;
    while (index < sz && (str[index] >= '0' && str[index] <= '9')) {
      ++index;
    }

    if (index < sz && (str[index] == '.' || str[index] == 'e' || str[index] == 'E')) {
      isFloat = true;
      char ch = str[index];
      ++index;
      if (ch == '.') {
        return checkFloatDotPart(str, index, suffix, ud_suffix);
      }
      return checkFloatExpPart(str, index, suffix, ud_suffix);
    }

    string::size_type i = index;
    if (!checkIntegerSuffix(str, i, suffix, ud_suffix)) {
      return false;
    }
    return i == sz;
  }

  bool checkFloatDotPart(const string &str, string::size_type &index,
                         string &suffix, string &ud_suffix) {
    auto sz = str.size();

    while (index < sz && (str[index] >= '0' && str[index] <= '9')) {
      ++index;
    }

    if (index < sz && (str[index] == 'e' || str[index] == 'E')) {
      ++index;
      return checkFloatExpPart(str, index, suffix, ud_suffix);
    }

    string::size_type i = index;
    if (!checkFloatSuffix(str, i, suffix, ud_suffix)) {
      return false;
    }
    return i == sz;
  }

  bool checkFloatExpPart(const string &str, string::size_type &index,
                         string &suffix, string &ud_suffix) {
    auto sz = str.size();
    if (index < sz && (str[index] == '+' || str[index] == '-')) {
      ++index;
    }

    while (index < sz && (str[index] >= '0' && str[index] <= '9')) {
      ++index;
    }

    string::size_type i = index;
    if (!checkFloatSuffix(str, i, suffix, ud_suffix)) {
      return false;
    }
    return i == sz;
  }

  bool checkFloatSuffix(const string &str, string::size_type &index,
                        string &suffix, string &ud_suffix) {
    auto sz = str.size();

    if (index < sz && (std::tolower(str[index]) == 'f' || std::tolower(str[index]) == 'l')) {
      suffix.push_back(str[index]);
      ++index;
      return true;
    } else if (index < sz && str[index] == '_') {
      // TODO: complete ud suffix
      return true;
    }
    return index == sz;
  }

  bool checkHexInteger(const string &str, string::size_type &index,
                       string &suffix, string &ud_suffix) {
    auto sz = str.size();
    if (sz == 2 || !isHex(str[2])) {
      return false;
    }
    index = 3;
    while (index < sz && isHex(str[index])) {
      ++index;
    }

    string::size_type i = index;
    if (!checkIntegerSuffix(str, i, suffix, ud_suffix)) {
      return false;
    }

    return i == sz;
  }

  bool checkIntegerSuffix(const string &str, string::size_type &index,
                          string &suffix, string &ud_suffix) {
    auto sz = str.size();

    if (index < sz && std::tolower(str[index]) == 'u') {
      suffix.push_back(str[index]);
      ++index;

      char long_suffix = '\0';
      if (index < sz && std::tolower(str[index]) == 'l') {
        long_suffix = str[index];
        suffix.push_back(str[index]);
        ++index;
      }

      if (index < sz && std::tolower(str[index]) == 'l') {
        suffix.push_back(str[index]);
        if (str[index] == long_suffix) {
          ++index;
        } else {
          return false;
        }
      }
      return true;
    } else if (index < sz && str[index] == '_') {
      ++index;

      // TODO: check ud suffix

      return true;
    }

    return index == sz;
  }

  bool checkOctInteger(const string &str, string::size_type &index,
                       string &suffix, string &ud_suffix) {

    auto sz = str.size();
    index = 1;
    while (index < sz && (str[index] >= '0' && str[index] <= '7')) {
      ++index;
    }

    string::size_type i = index;
    if (!checkIntegerSuffix(str, i, suffix, ud_suffix)) {
      return false;
    }
    return i == sz;
  }


  void process_HeaderName(const PPToken &token) {
    output.emit_invalid(token.data);
  }


private:
  DebugPostTokenOutputStream &output;
  std::list<PPToken> pending;
};

int main() {

  //freopen("input", "r", stdin);
  freopen("/home/syl/git/myproject/cppgm/pa2/tests/200-basic-floating.t", "r", stdin);

  try {
    ostringstream oss;
    oss << cin.rdbuf();

    string input = oss.str();

    DebugPostTokenOutputStream output;
    DebugPPTokenStream ppTokenStream;
    PPTokenizer tokenizer(ppTokenStream);
    PostTokenizer postTokenizer(output);

    for (char c : input) {
      auto code_unit = static_cast<unsigned char>(c);
      tokenizer.process(code_unit);

      while (!ppTokenStream.empty()) {
        postTokenizer.process(ppTokenStream.next());
      }
    }

    tokenizer.process(EndOfFile);
    while (!ppTokenStream.empty()) {
      postTokenizer.process(ppTokenStream.next());
    }

  } catch (exception &e) {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
  } catch (const char *e) {
    cerr << "ERROR: " << e << endl;
    return EXIT_FAILURE;
  }
}
