// (C) 2013 CPPGM Foundation. All Rights Reserved. www.cppgm.org

#pragma once

#include <string>
#include <list>
#include <vector>
#include <deque>
#include "IPPTokenStream.h"

#ifndef NDEBUG
#define ASSERT(cond, msg) do {\
  if (!(cond)) {\
    std::cerr << "error:" << __FILE__ << ":" << __LINE__ << "  " << (msg) << std::endl;\
    std::exit(EXIT_FAILURE);\
  }\
} while (false)
#else
#define ASSERT(cond, msg)
#endif

enum class PPTokenType : char {
  Tk_HeaderName,
  Tk_Identifier,
  Tk_PPNumber,
  Tk_CharacterLiteral,
  Tk_UdCharacterLiteral,
  Tk_StringLiteral,
  Tk_UdStringLiteral,
  Tk_OpOrPunc,
  Tk_NonWhitespaceChar,
  Tk_EOF,
  Tk_Null,
};

constexpr int EndOfFile = -1;

struct PPToken {
  PPTokenType type;
  std::string data;

  PPToken() : type(PPTokenType::Tk_Null) {}

  PPToken(PPTokenType t, const std::string &d) : type(t), data(d) {}
};

struct PPTokenizer {

  PPTokenizer(IPPTokenStream &output);

  void process(int c);

private:
  IPPTokenStream &output;
private:
  enum DecodeState {
    D_None,
    D_UTF8,
    D_LittleU,
    D_LargeU,
    D_ForwardSlash,
    D_BackSlash,
    D_MayBeTriGraph1,
    D_MayBeTriGraph2,
    D_SingleLineComment,
    D_InlineComment,
    D_MayEndInlineComment,
  };

  enum State {
    S_None,
    S_Identifier,
    S_HeaderName,
    S_PPNumber,
    S_PPNumberExpectSign,
    S_StartCharacterLiteral,
    S_EndCharacterLiteral,
    S_UserDefinedCharacterLiteral,
    S_StartNormalStringLiteral,
    S_EndNormalStringLiteral,
    S_UserDefinedNormalStringLiteral,
    S_StartRawStringLiteralDChar,
    S_StartRawStringLiteralRChar,
    S_MayBeEndRawStringLiteralRChar,
    S_EndRawStringLiteral,
    S_UserDefinedRawStringLiteral,
    S_StartOpOrPunc,
  };

  enum InnerState {
    Inner_None,
    Inner_BackSlash,
    Inner_Hex,
    Inner_Oct1,
    Inner_Oct2,
  };

private:
  void beginUTF8State(int c);

  bool decode_None(int c);

  bool decode_UTF8(int c);

  bool decode_ForwardSlash(int c);

  bool decode_BackSlash(int c);

  bool decode_UniversalCharacterName(DecodeState s, int c);

  bool decode_MayBeTriGraph1(int c);

  bool decode_MayBeTriGraph2(int c);

  void decode_SingleLineComment(int c);

  void decode_InlineComment(int c);

  void decode_MayEndInlineComment(int c);

  bool decode(int c);

  void step(int cp);

  void emit(int c, bool cont);

  void step_None(int c);

  void step_Identifier(int c);

  void step_PPNumber(int c);

  void step_PPNumberExpectSign(int c);

  void step_HeaderName(int c);

  std::vector<int> splitOpOrPunc(std::string &data, int c);

  void step_OpOrPunc(int c);

  void step_StartCharacterLiteral(int c);

  void step_EndCharacterLiteral(int c);

  void step_UserDefinedCharacterLiteral(int c);

  void step_StartNormalStringLiteral(int c);

  void step_EndNormalStringLiteral(int c);

  void step_UserDefinedNormalStringLiteral(int c);

  void step_StartRawStringLiteralDChar(int c);

  void step_StartRawStringLiteralRChar(int c);

  void step_MayBeEndRawStringLiteralRChar(int c);

  void step_EndRawStringLiteral(int c);

  void step_UserDefinedRawStringLiteral(int c);

  void step_UserDefinedSuffix(int c);

private:
  // variables for translation task
  std::string buffer_;
  int counts_;
  DecodeState decode_state_;
  std::deque<int> code_points_;
  int last_code_point_;
  int last_but_one_code_point_;
  bool is_prev_back_slash_;

  // variables for tokenization task
  std::vector<int> data_;

  bool is_prev_whitespace_;

  // used for header name
  bool is_prev_new_line_;
  bool is_prev_pound_key_;
  bool is_prev_include_;

  // used for normal string
  bool is_normal_string_mode_;

  // used for raw string
  bool is_raw_string_mode_;
  bool is_prev_right_paren_;
  std::vector<int> prefix_;
  std::vector<int>::size_type compare_index_;

  State state_;
  InnerState inner_state_;
};

struct DebugPPTokenStream : IPPTokenStream {
  void emit_whitespace_sequence() {
    /* do nothing */
  }

  void emit_new_line() {
    /* do nothing */
  }

  void emit_header_name(const std::string &data) {
    emit(PPTokenType::Tk_HeaderName, data);
  }

  void emit_identifier(const std::string &data) {
    emit(PPTokenType::Tk_Identifier, data);
  }

  void emit_pp_number(const std::string &data) {
    emit(PPTokenType::Tk_PPNumber, data);
  }

  void emit_character_literal(const std::string &data) {
    emit(PPTokenType::Tk_CharacterLiteral, data);
  }

  void emit_user_defined_character_literal(const std::string &data) {
    emit(PPTokenType::Tk_UdCharacterLiteral, data);
  }

  void emit_string_literal(const std::string &data) {
    emit(PPTokenType::Tk_StringLiteral, data);
  }

  void emit_user_defined_string_literal(const std::string &data) {
    emit(PPTokenType::Tk_UdStringLiteral, data);
  }

  void emit_preprocessing_op_or_punc(const std::string &data) {
    emit(PPTokenType::Tk_OpOrPunc, data);
  }

  void emit_non_whitespace_char(const std::string &data) {
    emit(PPTokenType::Tk_NonWhitespaceChar, data);
  }

  void emit_eof() {
    emit(PPTokenType::Tk_EOF, "");
  }

  PPToken next() {
    PPToken t = tokens.front();
    tokens.pop_front();
    return t;
  }

  bool empty() {
    return tokens.empty();
  }

private:
  std::list<PPToken> tokens;

  void emit(PPTokenType type, const std::string &data) {
    tokens.emplace_back(PPToken(type, data));
  }
};

bool isNonDigit(int c);
bool isInAnnexE1(int c);