#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <deque>

using namespace std;

#include "IPPTokenStream.h"
#include "DebugPPTokenStream.h"

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

// Translation features you need to implement:
// - utf8 decoder
// - utf8 encoder
// - universal-character-name decoder
// - trigraphs
// - line splicing
// - newline at eof
// - comment striping (can be part of whitespace-sequence)

// EndOfFile: synthetic "character" to represent the end of source file
constexpr int EndOfFile = -1;
constexpr int LF = 0x0A;

// given hex digit character c, return its value
int HexCharToValue(int c) {
  switch (c) {
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    case 'A':
      return 10;
    case 'a':
      return 10;
    case 'B':
      return 11;
    case 'b':
      return 11;
    case 'C':
      return 12;
    case 'c':
      return 12;
    case 'D':
      return 13;
    case 'd':
      return 13;
    case 'E':
      return 14;
    case 'e':
      return 14;
    case 'F':
      return 15;
    case 'f':
      return 15;
    default:
      throw logic_error("HexCharToValue of nonhex char");
  }
}

// See C++ standard 2.11 Identifiers and Appendix/Annex E.1
const vector<pair<int, int>> AnnexE1_Allowed_RangesSorted =
    {
        {0xA8,    0xA8},
        {0xAA,    0xAA},
        {0xAD,    0xAD},
        {0xAF,    0xAF},
        {0xB2,    0xB5},
        {0xB7,    0xBA},
        {0xBC,    0xBE},
        {0xC0,    0xD6},
        {0xD8,    0xF6},
        {0xF8,    0xFF},
        {0x100,   0x167F},
        {0x1681,  0x180D},
        {0x180F,  0x1FFF},
        {0x200B,  0x200D},
        {0x202A,  0x202E},
        {0x203F,  0x2040},
        {0x2054,  0x2054},
        {0x2060,  0x206F},
        {0x2070,  0x218F},
        {0x2460,  0x24FF},
        {0x2776,  0x2793},
        {0x2C00,  0x2DFF},
        {0x2E80,  0x2FFF},
        {0x3004,  0x3007},
        {0x3021,  0x302F},
        {0x3031,  0x303F},
        {0x3040,  0xD7FF},
        {0xF900,  0xFD3D},
        {0xFD40,  0xFDCF},
        {0xFDF0,  0xFE44},
        {0xFE47,  0xFFFD},
        {0x10000, 0x1FFFD},
        {0x20000, 0x2FFFD},
        {0x30000, 0x3FFFD},
        {0x40000, 0x4FFFD},
        {0x50000, 0x5FFFD},
        {0x60000, 0x6FFFD},
        {0x70000, 0x7FFFD},
        {0x80000, 0x8FFFD},
        {0x90000, 0x9FFFD},
        {0xA0000, 0xAFFFD},
        {0xB0000, 0xBFFFD},
        {0xC0000, 0xCFFFD},
        {0xD0000, 0xDFFFD},
        {0xE0000, 0xEFFFD}
    };

// See C++ standard 2.11 Identifiers and Appendix/Annex E.2
const vector<pair<int, int>> AnnexE2_DisallowedInitially_RangesSorted =
    {
        {0x300,  0x36F},
        {0x1DC0, 0x1DFF},
        {0x20D0, 0x20FF},
        {0xFE20, 0xFE2F}
    };

// See C++ standard 2.13 Operators and punctuators
const unordered_set<string> Digraph_IdentifierLike_Operators =
    {
        "new", "delete", "and", "and_eq", "bitand",
        "bitor", "compl", "not", "not_eq", "or",
        "or_eq", "xor", "xor_eq"
    };

// See `simple-escape-sequence` grammar
const unordered_set<int> SimpleEscapeSequence_CodePoints =
    {
        '\'', '"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v'
    };


const unordered_set<int> SingleCharacter_Op_or_Punc =
    {
        '{', '}', '[', ']', '#', '(', ')', ';', ':', '?', '.',
        '+', '-', '*', '/', '%', '^', '&', '|', '~', '!', '=', '<', '>', ',',
    };

const unordered_set<string> TwoCharacter_Op_or_Punc =
    {
        "##", "<:", ":>", "<%", "%>", "%:", "::", ".*", "->",
        "+=", "-=", "*=", "/=", "%=", "^=", "&=", "|=", "==",
        "!=", "<=", ">=", "&&", "||", "<<", ">>", "++", "--",
    };

const unordered_set<string> ThreeCharacter_Op_or_Punc =
    {
        "...", "->*", "<=>", "<<=", ">>=", "<::",
    };

const unordered_set<string> FourCharacter_Op_or_Punc =
    {
        "%:%:", "<::>", "<:::",
    };


static bool isCharacterLiteralPrefix(const vector<int> &data) {
  return data.size() == 1 && (
      data.back() == 'u' || data.back() == 'U' || data.back() == 'L');
}

static bool isNormalStringLiteralPrefix(const vector<int> &data) {
  auto sz = data.size();
  return (sz == 1 && (data.front() == 'u' || data.front() == 'U' || data.front() == 'L')) ||
         (sz == 2 && data.front() == 'u' && data.back() == '8');
}

static bool isRawStringLiteralPrefix(const vector<int> &data) {
  auto sz = data.size();
  return (sz == 1 && data.front() == 'R') ||
         (sz == 2 && (data.front() == 'u' || data.front() == 'U') && data.back() == 'R') ||
         (sz == 3 && data[0] == 'u' && data[1] == '8' && data[2] == 'R');
}

static int str2int(const string &str, int base) {
  if (str.empty()) {
    return 0;
  }
  std::size_t pos;
  int v = std::stoi(str, &pos, base);
  ASSERT(pos == str.size(), "string must be converted to integer");
  return v;
}

static inline bool isHex(int c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}


static int toCodePoint(const string &text) {
  return str2int(text, 16);
}

static inline bool isUtf8Trailing(int c) {
  return ((c >> 6) & 0x03) == 0x02;
}

static inline bool isNonDigit(int c) {
  return (c == '_') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool isInAnnexE1(int c) {
  // TODO: Optimize Query Speed
  for (const auto &p : AnnexE1_Allowed_RangesSorted) {
    if (c >= p.first && c <= p.second) {
      return true;
    }
  }
  return false;
}

static inline bool isInAnnexE2(int c) {
  // TODO: Optimize Query Speed
  for (const auto &p : AnnexE2_DisallowedInitially_RangesSorted) {
    if (c >= p.first && c <= p.second) {
      return true;
    }
  }
  return false;
}

static inline bool isIdentifierNonDigit(int c) {
  return (isNonDigit(c) || isInAnnexE1(c)) && !isInAnnexE2(c);
}

static inline string codePoint2String(int c) {
  if (c < 0 || c > 0x10FFFF) {
    throw "invalid code point";
  }
  string data;

  char c1, c2, c3, c4;
  if (c >= 0 && c <= 0x7f) {
    data.push_back(static_cast<char>(c));
  } else if (c <= 0x7ff) {
    c1 = static_cast<char>(((c >> 6) & 0x1f) | 0xc0);
    c2 = static_cast<char>((c & 0x3f) | 0x80);
    data.push_back(c1);
    data.push_back(c2);
  } else if (c <= 0xffff) {
    c1 = static_cast<char>(((c >> 12) & 0x0f) | 0xe0);
    c2 = static_cast<char>(((c >> 6) & 0x3f) | 0x80);
    c3 = static_cast<char>((c & 0x3f) | 0x80);
    data.push_back(c1);
    data.push_back(c2);
    data.push_back(c3);
  } else {
    c1 = static_cast<char>(((c >> 18) & 0x07) | 0xf0);
    c2 = static_cast<char>(((c >> 12) & 0x3f) | 0x80);
    c3 = static_cast<char>(((c >> 6) & 0x3f) | 0x80);
    c4 = static_cast<char>((c & 0x3f) | 0x80);
    data.push_back(c1);
    data.push_back(c2);
    data.push_back(c3);
    data.push_back(c4);
  }
  return data;
}

static string codePoints2String(const vector<int> &cps) {
  string data;
  for (const int c : cps) {
    data.append(codePoint2String(c));
  }
  return data;
}


// Tokenizer
struct PPTokenizer {
  IPPTokenStream &output;


  PPTokenizer(IPPTokenStream &output)
      : output(output) {
    state_ = S_None;
    inner_state_ = Inner_None;
    decode_state_ = D_None;
    last_code_point_ = -1;
    last_but_one_code_point_ = -1;
    is_normal_string_mode_ = false;
    is_raw_string_mode_ = false;
    is_prev_whitespace_ = false;
    is_prev_new_line_ = true;
    is_prev_pound_key_ = false;
    is_prev_include_ = false;
    is_prev_right_paren_ = false;
    compare_index_ = 0;
  }

  void process(int c) {

    // 1. do translation features
    // 2. tokenize resulting stream
    // 3. call an output.emit_* function for each token.

    int cp;

    if (decode(c)) {
      cp = code_points_.front();
      code_points_.pop_front();
      step(cp);
      last_but_one_code_point_ = last_code_point_;
      last_code_point_ = cp;
    }

    if (c == EndOfFile) {
      while (!code_points_.empty()) {
        cp = code_points_.front();
        code_points_.pop_front();
        step(cp);
        last_but_one_code_point_ = last_code_point_;
        last_code_point_ = cp;
      }
    }


    // TIP: Reference implementation is about 1000 lines of code.
    // It is a state machine with about 50 states, most of which
    // are simple transitions of the operators.
  }


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

  void beginUTF8State(int c) {
    decode_state_ = D_UTF8;
    counts_ = 0;

    int shift = 7;
    while ((shift >= 4) && ((c >> shift) & 0x01)) {
      shift--;
      counts_++;
    }

    if (shift < 3) {
      throw "utf8 invalid unit (11111xx)";
    } else if (shift > 5) {
      throw "utf8 trailing code unit (10xxxxxx) at start";
    } else {
      --counts_;
      ASSERT(buffer_.empty(), "buffer must be empty");
      buffer_.push_back((char) (c & (~(0xff << shift))));
    }
  }

  bool decode_None(int c) {
    ASSERT(buffer_.empty(), "buffer must be empty");

    bool ret = false;

    if (c < 0x7f) {
      if (c == '/') {
        decode_state_ = D_ForwardSlash;
      } else if (c == '\\') {
        decode_state_ = D_BackSlash;
      } else if (c == '?') {
        decode_state_ = D_MayBeTriGraph1;
      } else {
        code_points_.push_back(c);
        ret = true;
      }
    } else {
      beginUTF8State(c);
    }

    return ret;
  }

  bool decode_UTF8(int c) {

    bool ret = false;

    if (isUtf8Trailing(c)) {
      ASSERT(counts_ > 0, "the count of remaining trailing bytes must be greater than 0");
      buffer_.push_back((char) (c & 0x3f));
      --counts_;
      if (counts_ == 0) {

        auto sz = buffer_.size(), shift = 6 * (sz - 1);
        int code = (int) (buffer_.front()) << shift;

        for (auto i = 1u; i < sz; i++) {
          shift -= 6;
          code |= ((int) (buffer_[i]) << shift);
        }
        code_points_.push_back(code);
        decode_state_ = D_None;
        buffer_.clear();
        ret = true;
      }
    } else {
      throw "utf8 expected trailing byte (10xxxxxx)";
    }

    return ret;
  }

  bool decode_ForwardSlash(int c) {

    bool ret = false;

    if (is_normal_string_mode_ || is_raw_string_mode_) {
      code_points_.push_back('/');
      decode_state_ = D_None;
      decode(c);
      ret = true;
    } else if (c == '/') {
      // line comment
      decode_state_ = D_SingleLineComment;
      is_prev_back_slash_ = false;

      if (state_ != S_None) {
        emit(c, false);
      }
    } else if (c == '*') {
      decode_state_ = D_InlineComment;
    } else {
      code_points_.push_back('/');
      decode_state_ = D_None;
      decode(c);
      ret = true;
    }

    return ret;
  }

  bool decode_BackSlash(int c) {

    bool ret = false;

    if (is_raw_string_mode_) {
      code_points_.push_back('\\');
      decode_state_ = D_None;
      decode(c);
      ret = true;
    } else if (c == LF) {
      // line splicing
      decode_state_ = D_None;
    } else if (c == 'u') {
      ASSERT(buffer_.empty(), "buffer must be empty");
      decode_state_ = D_LittleU;
    } else if (c == 'U') {
      ASSERT(buffer_.empty(), "buffer must be empty");
      decode_state_ = D_LargeU;
    } else {
      code_points_.push_back('\\');
      decode_state_ = D_None;
      decode(c);
      ret = true;
    }

    return ret;
  }

  bool decode_UniversalCharacterName(DecodeState s, int c) {

    bool ret = false;
    // universal character name decoding
    if (isHex(c) && !is_raw_string_mode_) {
      buffer_.push_back(static_cast<char>(c));
      if (s == D_LittleU && buffer_.size() == 4) {
        int cp = toCodePoint(buffer_);
        code_points_.push_back(cp);
        buffer_.clear();
        decode_state_ = D_None;
        ret = true;
      } else if (s == D_LargeU && buffer_.size() == 8) {
        int cp = toCodePoint(buffer_);
        code_points_.push_back(cp);
        buffer_.clear();
        decode_state_ = D_None;
        ret = true;
      }
    } else {
      code_points_.push_back('\\');
      if (s == D_LittleU) {
        code_points_.push_back('u');
      } else {
        code_points_.push_back('U');
      }
      string buf(buffer_);
      buffer_.clear();
      decode_state_ = D_None;
      for (const auto x : buf) {
        decode(x);
      }
      decode(c);
      ret = true;
    }

    return ret;
  }

  bool decode_MayBeTriGraph1(int c) {

    ASSERT(!is_raw_string_mode_, "cannot reach here if in raw string mode");

    bool ret = false;
    // tri-graph decoding
    if (c == '?') {
      decode_state_ = D_MayBeTriGraph2;
    } else {
      code_points_.push_back('?');
      decode_state_ = D_None;
      decode(c);
      ret = true;
    }

    return ret;
  }

  bool decode_MayBeTriGraph2(int c) {

    ASSERT(!is_raw_string_mode_, "cannot reach here if in raw string mode");

    bool ret = false;
    int cp = -1;

    switch (c) {
      case '=':
        cp = '#';
        break;
      case '/':
        cp = '\\';
        break;
      case '\'':
        cp = '^';
        break;
      case '(':
        cp = '[';
        break;
      case ')':
        cp = ']';
        break;
      case '!':
        cp = '|';
        break;
      case '<':
        cp = '{';
        break;
      case '>':
        cp = '}';
        break;
      case '-':
        cp = '~';
        break;
      default:
        if (c == '?') {
          code_points_.push_back('?');
        } else {
          code_points_.push_back('?');
          code_points_.push_back('?');
          decode_state_ = D_None;
          decode(c);
        }

        ret = true;
        break;
    }
    if (cp != -1) {
      decode_state_ = D_None;
      if (cp == '\\') {
        decode(cp);
      } else {
        code_points_.push_back(cp);
        ret = true;
      }
    }

    return ret;
  }

  void decode_SingleLineComment(int c) {
    if (c == EndOfFile) {
      code_points_.push_back(' ');
      code_points_.push_back(c);
    } else if (c == LF) {
      if (is_prev_back_slash_) {
        is_prev_back_slash_ = false;
      } else {
        decode_state_ = D_None;
        code_points_.push_back(' ');
        code_points_.push_back(LF);
        is_prev_new_line_ = true;
      }
    } else if (c == '\\') {
      is_prev_back_slash_ = true;
    } else {
      is_prev_back_slash_ = false;
    }
  }

  void decode_InlineComment(int c) {
    if (c == EndOfFile) {
      throw "partial comment";
    }
    if (c == '*') {
      decode_state_ = D_MayEndInlineComment;
    }
  }

  void decode_MayEndInlineComment(int c) {
    if (c == EndOfFile) {
      throw "partial comment";
    }
    if (c == '/') {
      decode_state_ = D_None;
      code_points_.push_back(' ');
    } else {
      decode_state_ = D_InlineComment;
    }
  }

  bool decode(int c) {

    DecodeState s = decode_state_;
    bool ret = false;

    if (is_raw_string_mode_ && s != D_UTF8) {
      if (c < 0x7f) {
        code_points_.push_back(c);
        ret = true;
      } else {
        beginUTF8State(c);
      }
      return ret;
    }

    switch (s) {
      case D_None:
        ret = decode_None(c);
        break;
      case D_UTF8:
        ret = decode_UTF8(c);
        break;
      case D_ForwardSlash:
        ret = decode_ForwardSlash(c);
        break;
      case D_BackSlash:
        ret = decode_BackSlash(c);
        break;
      case D_LittleU:
      case D_LargeU:
        ret = decode_UniversalCharacterName(s, c);
        break;
      case D_MayBeTriGraph1:
        ret = decode_MayBeTriGraph1(c);
        break;
      case D_MayBeTriGraph2:
        ret = decode_MayBeTriGraph2(c);
        break;
      case D_SingleLineComment:
        decode_SingleLineComment(c);
        break;
      case D_InlineComment:
        decode_InlineComment(c);
        break;
      case D_MayEndInlineComment:
        decode_MayEndInlineComment(c);
        break;
      default:
        ASSERT(false, "invalid statement");
    }

    // file terminating line-ending
    if (!ret && !code_points_.empty()) {
      ret = true;
    }
    return ret;
  }

  void step(int cp) {

    switch (state_) {
      case S_None:
        step_None(cp);
        break;
      case S_Identifier:
        step_Identifier(cp);
        break;
      case S_PPNumber:
        step_PPNumber(cp);
        break;
      case S_PPNumberExpectSign:
        step_PPNumberExpectSign(cp);
        break;
      case S_HeaderName:
        step_HeaderName(cp);
        break;
      case S_StartOpOrPunc:
        step_OpOrPunc(cp);
        break;
      case S_StartCharacterLiteral:
        step_StartCharacterLiteral(cp);
        break;
      case S_EndCharacterLiteral:
        step_EndCharacterLiteral(cp);
        break;
      case S_UserDefinedCharacterLiteral:
        step_UserDefinedCharacterLiteral(cp);
        break;
      case S_StartNormalStringLiteral:
        step_StartNormalStringLiteral(cp);
        break;
      case S_EndNormalStringLiteral:
        step_EndNormalStringLiteral(cp);
        break;
      case S_UserDefinedNormalStringLiteral:
        step_UserDefinedNormalStringLiteral(cp);
        break;
      case S_StartRawStringLiteralDChar:
        step_StartRawStringLiteralDChar(cp);
        break;
      case S_StartRawStringLiteralRChar:
        step_StartRawStringLiteralRChar(cp);
        break;
      case S_MayBeEndRawStringLiteralRChar:
        step_MayBeEndRawStringLiteralRChar(cp);
        break;
      case S_EndRawStringLiteral:
        step_EndRawStringLiteral(cp);
        break;
      case S_UserDefinedRawStringLiteral:
        step_UserDefinedRawStringLiteral(cp);
        break;
      default:
        ASSERT(false, "invalid state");
    }
  }

  void emit(int c, bool cont) {

    string data;

    switch (state_) {
      case S_Identifier: {
        data = codePoints2String(data_);

        if (Digraph_IdentifierLike_Operators.find(data) != Digraph_IdentifierLike_Operators.end()) {
          output.emit_preprocessing_op_or_punc(data);
        } else {
          output.emit_identifier(data);
        }

        if (is_prev_pound_key_) {
          is_prev_pound_key_ = false;
          if (data == "include") {
            is_prev_include_ = true;
          }
        }
        break;
      }
      case S_HeaderName: {
        data = codePoints2String(data_);
        output.emit_header_name(data);
        is_prev_include_ = false;
        break;
      }
      case S_EndCharacterLiteral: {
        data = codePoints2String(data_);
        output.emit_character_literal(data);
        break;
      }
      case S_UserDefinedCharacterLiteral: {
        data = codePoints2String(data_);
        output.emit_user_defined_character_literal(data);
        break;
      }

      case S_EndNormalStringLiteral:
      case S_EndRawStringLiteral: {
        data = codePoints2String(data_);
        output.emit_string_literal(data);
        break;
      }

      case S_UserDefinedNormalStringLiteral:
      case S_UserDefinedRawStringLiteral: {
        data = codePoints2String(data_);
        output.emit_user_defined_string_literal(data);
        break;
      }

      case S_PPNumber:
      case S_PPNumberExpectSign: {
        data = codePoints2String(data_);
        output.emit_pp_number(data);
        break;
      }

      case S_StartOpOrPunc: {
        data = codePoints2String(data_);
        is_prev_pound_key_ = false;
        if (is_prev_new_line_ && data == "#") {
          is_prev_pound_key_ = true;
        }
        output.emit_preprocessing_op_or_punc(data);
        break;
      }
      default:
        ASSERT(false, "invalid state");
    }

    is_prev_new_line_ = false;
    state_ = S_None;
    data_.clear();
    if (cont) {
      step_None(c);
    }
  }

  void step_None(int c) {

    ASSERT(data_.empty(), "buffer must be empty");

    if (c < 0x7f && std::isspace(c) && c != LF) {
      if (!is_prev_whitespace_) {
        output.emit_whitespace_sequence();
        is_prev_whitespace_ = true;
      }
      return;
    }
    is_prev_whitespace_ = false;

    if (c == EndOfFile) {
      if ((last_code_point_ != -1 && last_code_point_ != LF) ||
          (last_code_point_ == LF && last_but_one_code_point_ == '\\')) {
        output.emit_new_line();
        is_prev_new_line_ = true;
      }
      output.emit_eof();
    } else if (c == LF) {
      output.emit_new_line();
      is_prev_new_line_ = true;
    } else if (isIdentifierNonDigit(c)) {
      // identifier
      state_ = S_Identifier;
      data_.push_back(c);
    } else if (std::isdigit(c)) {
      // pp-number
      state_ = S_PPNumber;
      data_.push_back(c);
    } else if ('\'' == c) {
      // character-literal or user-defined-character-literal
      state_ = S_StartCharacterLiteral;
      data_.push_back(c);
    } else if ('"' == c) {
      // string-literal or user-defined-string-literal
      if (is_prev_include_) {
        state_ = S_HeaderName;
      } else {
        state_ = S_StartNormalStringLiteral;
        is_normal_string_mode_ = true;
      }
      data_.push_back(c);
    } else if (SingleCharacter_Op_or_Punc.find(c) != SingleCharacter_Op_or_Punc.end()) {
      // preprocessing-op-or-punc
      if (is_prev_include_) {
        state_ = S_HeaderName;
      } else {
        state_ = S_StartOpOrPunc;
      }
      data_.push_back(c);
    } else {
      // each non-white-space character that cannot be one of the above
      string data = codePoint2String(c);
      output.emit_non_whitespace_char(data);
    }
  }

  void step_Identifier(int c) {
    if (isNonDigit(c) || std::isdigit(c) || isInAnnexE1(c)) {
      data_.push_back(c);
    } else if ('\'' == c && isCharacterLiteralPrefix(data_)) {
      data_.push_back(c);
      state_ = S_StartCharacterLiteral;
      inner_state_ = Inner_None;
    } else if ('"' == c && isNormalStringLiteralPrefix(data_)) {
      data_.push_back(c);
      state_ = S_StartNormalStringLiteral;
      is_normal_string_mode_ = true;
      inner_state_ = Inner_None;
    } else if ('"' == c && isRawStringLiteralPrefix(data_)) {
      data_.push_back(c);
      state_ = S_StartRawStringLiteralDChar;
      is_raw_string_mode_ = true;
      inner_state_ = Inner_None;
    } else {
      emit(c, true);
    }
  }

  void step_PPNumber(int c) {
    if (c == 'E' || c == 'e') {
      data_.push_back(c);
      state_ = S_PPNumberExpectSign;
    } else if (c == '.' || std::isdigit(c) || isIdentifierNonDigit(c)) {
      data_.push_back(c);
    } else {
      emit(c, true);
    }
  }

  void step_PPNumberExpectSign(int c) {
    if (c == '+' || c == '-' || c == '.' || std::isdigit(c) || isIdentifierNonDigit(c)) {
      data_.push_back(c);
      state_ = S_PPNumber;
    } else {
      emit(c, true);
    }
  }

  void step_HeaderName(int c) {
    ASSERT(!data_.empty() && (data_.front() == '<' || data_.front() == '"'),
           "incorrect header name buffer");
    data_.push_back(c);
    if ((data_.front() == '<' && c == '>') || (data_.front() == '"' && c == '"')) {
      emit(c, false);
    }
  }

  vector<int> splitOpOrPunc(string &data, int c) {
    vector<int> remaining;

    while (!data.empty()) {
      if (data.size() == 4 && FourCharacter_Op_or_Punc.find(data) != FourCharacter_Op_or_Punc.end()) {
        if (data == "<::>" || data == "<:::") {
          ASSERT(data_.size() == 4, "buffer size must be 4");
          for (auto i = 0; i < 2; i++) {
            remaining.push_back(data_.back());
            data_.pop_back();
          }
        }
        emit(c, false);
        break;
      } else if (data.size() == 3 && ThreeCharacter_Op_or_Punc.find(data) != ThreeCharacter_Op_or_Punc.end()) {

        if (data == "<::") {
          ASSERT(data_.size() >= 3, "buffer size must greater than or equal to 3");
          for (auto i = 0; i < 2; i++) {
            remaining.push_back(data_.back());
            data_.pop_back();
          }
        }
        while (data_.size() > 3) {
          remaining.push_back(data_.back());
          data_.pop_back();
        }


        emit(c, false);
        break;
      } else if (data.size() == 2 && TwoCharacter_Op_or_Punc.find(data) != TwoCharacter_Op_or_Punc.end()) {
        while (data_.size() > 2) {
          remaining.push_back(data_.back());
          data_.pop_back();
        }

        emit(c, false);
        break;
      } else if (data.size() == 1) {
        while (data_.size() > 1) {
          remaining.push_back(data_.back());
          data_.pop_back();
        }

        emit(c, false);
        break;
      }

      data.pop_back();
      remaining.push_back(data_.back());
      data_.pop_back();
    }

    return remaining;
  }

  void step_OpOrPunc(int c) {

    string data;
    vector<int> buf;

    data_.push_back(c);

    if (SingleCharacter_Op_or_Punc.find(c) != SingleCharacter_Op_or_Punc.end()) {
      data = codePoints2String(data_);
      if (data.size() == 4) {
        buf = splitOpOrPunc(data, c);
        state_ = S_None;
        std::reverse(buf.begin(), buf.end());
        for (const int cp : buf) {
          step(cp);
        }
      }
    } else {
      data_.pop_back();
      if (data_.size() == 1 && data_.front() == '.' && c < 0x7f && std::isdigit(c)) {
        state_ = S_PPNumber;
        data_.push_back(c);
      } else {
        data = codePoints2String(data_);
        buf = splitOpOrPunc(data, c);
        state_ = S_None;
        std::reverse(buf.begin(), buf.end());
        for (const int cp : buf) {
          step(cp);
        }
        step(c);
      }

    }
  }

  void step_StartCharacterLiteral(int c) {
    if (LF == c) {
      throw "unterminated character literal";
    }
    switch (inner_state_) {
      case Inner_None:
        data_.push_back(c);
        if (c == '\'') {
          state_ = S_EndCharacterLiteral;
        } else if (c == '\\') {
          inner_state_ = Inner_BackSlash;
        }
        break;
      case Inner_BackSlash:
        data_.push_back(c);
        if (c == 'x') {
          inner_state_ = Inner_Hex;
        } else if (SimpleEscapeSequence_CodePoints.find(c) != SimpleEscapeSequence_CodePoints.end()) {
          inner_state_ = Inner_None;
        } else if (c >= '0' && c <= '7') {
          inner_state_ = Inner_Oct1;
        } else {
          throw "invalid escape sequence";
        }
        break;
      case Inner_Oct1:
        if (c >= '0' && c <= '7') {
          data_.push_back(c);
          inner_state_ = Inner_Oct2;
        } else {
          inner_state_ = Inner_None;
          step_StartCharacterLiteral(c);
        }
        break;
      case Inner_Oct2:
        inner_state_ = Inner_None;
        if (c >= '0' && c <= '7') {
          data_.push_back(c);
        } else {
          step_StartCharacterLiteral(c);
        }
        break;
      default:
        ASSERT(false, "invalid inner state");
    }
  }

  void step_EndCharacterLiteral(int c) {

    ASSERT(state_ == S_EndCharacterLiteral, "current state must be S_EndCharacterLiteral");

    if (isIdentifierNonDigit(c)) {
      data_.push_back(c);
      state_ = S_UserDefinedCharacterLiteral;
    } else {
      emit(c, true);
    }
  }

  void step_UserDefinedCharacterLiteral(int c) {
    step_UserDefinedSuffix(c);
  }

  void step_StartNormalStringLiteral(int c) {
    if (LF == c) {
      throw "unterminated string literal";
    }

    switch (inner_state_) {
      case Inner_None:
        data_.push_back(c);
        if (c == '"') {
          state_ = S_EndNormalStringLiteral;
          is_normal_string_mode_ = false;
        } else if (c == '\\') {
          inner_state_ = Inner_BackSlash;
        }
        break;
      case Inner_BackSlash:
        data_.push_back(c);
        if (c == 'x') {
          inner_state_ = Inner_Hex;
        } else if (SimpleEscapeSequence_CodePoints.find(c) != SimpleEscapeSequence_CodePoints.end()) {
          inner_state_ = Inner_None;
        } else if (c >= '0' && c <= '7') {
          inner_state_ = Inner_Oct1;
        } else {
          throw "invalid escape sequence";
        }
        break;
      case Inner_Oct1:
        if (c >= '0' && c <= '7') {
          data_.push_back(c);
          inner_state_ = Inner_Oct2;
        } else {
          inner_state_ = Inner_None;
          step_StartNormalStringLiteral(c);
        }
        break;
      case Inner_Oct2:
        inner_state_ = Inner_None;
        if (c >= '0' && c <= '7') {
          data_.push_back(c);
        } else {
          step_StartNormalStringLiteral(c);
        }
        break;
      default:
        ASSERT(false, "invalid inner state");
    }
  }

  void step_EndNormalStringLiteral(int c) {
    ASSERT(state_ == S_EndNormalStringLiteral, "current state must be S_EndNormalStringLiteral");

    if (isIdentifierNonDigit(c)) {
      data_.push_back(c);
      state_ = S_UserDefinedNormalStringLiteral;
    } else {
      emit(c, true);
    }
  }

  void step_UserDefinedNormalStringLiteral(int c) {
    step_UserDefinedSuffix(c);
  }

  void step_StartRawStringLiteralDChar(int c) {
    if (std::isspace(c) || c == ')' || c == '\\' || c == '"') {
      throw "invalid characters in raw string delimiter";
    }
    data_.push_back(c);
    if (c == '(') {
      state_ = S_StartRawStringLiteralRChar;
    } else {
      prefix_.push_back(c);
    }
  }

  void step_StartRawStringLiteralRChar(int c) {

    ASSERT(!data_.empty(), "buffer must not be empty");

    data_.push_back(c);

    if (is_prev_right_paren_) {
      if (std::isspace(c) || c == ')' || c == '(' || c == '\\') {
        if (c != ')') {
          is_prev_right_paren_ = false;
        }
      } else {
        is_prev_right_paren_ = false;

        ASSERT(compare_index_ == 0, "compare index must be zero");

        if (compare_index_ == prefix_.size()) {
          if (c == '"') {
            state_ = S_EndRawStringLiteral;
            is_raw_string_mode_ = false;
          } else {
            throw "unterminated raw string literal";
          }
        } else {
          if (c == prefix_[compare_index_]) {
            compare_index_++;
            state_ = S_MayBeEndRawStringLiteralRChar;
          } else {
            compare_index_ = 0;
          }
        }
      }
    } else {
      if (c == ')') {
        is_prev_right_paren_ = true;
      }
    }
  }

  void step_MayBeEndRawStringLiteralRChar(int c) {

    data_.push_back(c);

    if (compare_index_ == prefix_.size()) {
      if (c == '"') {
        state_ = S_EndRawStringLiteral;
        is_raw_string_mode_ = false;
      } else {
        compare_index_ = 0;
        state_ = S_StartRawStringLiteralRChar;
      }
    } else {
      if (c == prefix_[compare_index_]) {
        compare_index_++;
      } else {
        compare_index_ = 0;
        state_ = S_StartRawStringLiteralRChar;
      }
    }
  }

  void step_EndRawStringLiteral(int c) {

    if (isIdentifierNonDigit(c)) {
      data_.push_back(c);
      state_ = S_UserDefinedNormalStringLiteral;
    } else {
      emit(c, true);
    }
  }

  void step_UserDefinedRawStringLiteral(int c) {
    step_UserDefinedSuffix(c);
  }

  void step_UserDefinedSuffix(int c) {
    if (isNonDigit(c) || std::isdigit(c) || isInAnnexE1(c)) {
      data_.push_back(c);
    } else {
      emit(c, true);
    }
  }

private:
  // variables for translation task
  string buffer_;
  int counts_;
  DecodeState decode_state_;
  deque<int> code_points_;
  int last_code_point_;
  int last_but_one_code_point_;
  bool is_prev_back_slash_;

  // variables for tokenization task
  vector<int> data_;

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
  vector<int> prefix_;
  vector<int>::size_type compare_index_;


  State state_;
  InnerState inner_state_;
};

int main() {

  try {
    ostringstream oss;
    oss << cin.rdbuf();

    string input = oss.str();

    DebugPPTokenStream output;

    PPTokenizer tokenizer(output);

    for (char c : input) {
      auto code_unit = static_cast<unsigned char>(c);
      tokenizer.process(code_unit);
    }

    tokenizer.process(EndOfFile);
  } catch (exception &e) {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
  } catch (const char *e) {
    cerr << "ERROR: " << e << endl;
    return EXIT_FAILURE;
  }
}

