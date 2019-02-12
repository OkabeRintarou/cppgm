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

const unordered_map<int, int> SimpleEscapeSequence_Map =
    {
        {'\'', 0x27},
        {'"',  0x22},
        {'?',  0x3f},
        {'\\', 0x5c},
        {'a',  0x07},
        {'b',  0x08},
        {'f',  0x0c},
        {'n',  0x0a},
        {'r',  0x0d},
        {'t',  0x09},
        {'v',  0x0b},
    };


const unordered_set<int> SingleCharacter_Op_or_Punc =
    {
        '{', '}', '[', ']', '#', '(', ')', ';', ':', '?', '.',
        '+', '-', '*', '/', '%', '^', '&', '|', '~', '!', '=', '<', '>',
    };

const unordered_set<string> MultipleCharacters_Op_or_Punc = {

};

enum TokenType {
  Tk_Null,
  Tk_Id,
  Tk_HeaderName,
  Tk_PPNumber,
  Tk_CharacterLiteral,
  Tk_UserDefinedCharacterLiteral,
  Tk_StringLiteral,
  Tk_UserDefinedStringLiteral,
};

enum State {
  S_None,
  S_Id,
  S_HeaderName,
  S_PPNumber,
  S_CharacterLiteral,
  S_UserDefinedCharacterLiteral,
  S_StringLiteral,
  S_UserDefinedStringLiteral,
  S_EncodingPrefiex,
  S_OpOrPunc,
};

enum StringState {
  Inner_None,
  Inner_BackSlash,
  Inner_Hex,
  Inner_Oct1,
  Inner_Oct2,
  Inner_Little_U,
  Inner_Large_U,
};


static const char *kMissingStringTerminator = "unterminated string literal";
static const char *kInvalidEscapeSequence = "invalid escape sequence";


struct Token {
  TokenType type;
  string text;

  Token(TokenType t, string txt) : type(t), text(std::move(txt)) {}
};


static bool startsWith(const string &s, const string &prefix) {
  if (prefix.size() > s.size()) {
    return false;
  }
  return strncmp(s.c_str(), prefix.c_str(), prefix.size()) == 0;
}

static bool isEncodingPrefix(const string &text) {
  assert(!text.empty());
  return startsWith(text, "u8") ||
         (text.size() == 1 && (text[0] == 'u' || text[0] == 'U' || text[0] == 'L'));
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

static string toUtf8(const string &text) {

  int v = str2int(text, 16);
  string str;

  while (v > 0x7f) {

  }


  return str;
}

static int toCodePoint(const string &text) {
  return 0;
}

static inline string littleU2Utf8(const string &text) {
  ASSERT(text.size() == 4, "string length must be 4");
  return toUtf8(text);
}

static inline string largeU2Utf8(const string &text) {
  ASSERT(text.size() == 8, "string length must be 8");
  string str;
  str.append(toUtf8(text.substr(0, 4)));
  str.append(toUtf8(text.substr(4, 4)));
  return str;
}

static inline bool isUtf8Trailing(int c) {
  return ((c >> 6) & 0x03) == 0x02;
}


// Tokenizer
struct PPTokenizer {
  IPPTokenStream &output;


  PPTokenizer(IPPTokenStream &output)
      : output(output) {
    state_ = S_None;
    prev_token_type_ = Tk_Null;
    string_inner_state_ = Inner_None;
    decode_state_ = D_None;
    expected_char_ = '\0';
    is_raw_string_ = false;
    is_prev_whitespace = false;
    is_escape_ = false;
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
    }


    // TIP: Reference implementation is about 1000 lines of code.
    // It is a state machine with about 50 states, most of which
    // are simple transitions of the operators.
  }


  void step(int cp) {
    printf("%x\n", cp);
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
private:

  bool decode(int c) {

    DecodeState s = decode_state_;
    bool ret = false;

    if (s == D_None) {

      ASSERT(buffer_.empty(), "buffer must be empty");

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
    } else if (s == D_UTF8) {
      // UTF-8 decoding

      if (isUtf8Trailing(c)) {
        ASSERT(counts_ > 0, "the count of remaining trailing bytes must be greater than 0");
        buffer_.push_back((char) (c & 0x3f));
        --counts_;
        if (counts_ == 0) {

          auto sz = buffer_.size(), shift = 6 * (sz - 1);
          int code = (int) (buffer_.front()) << shift;

          for (auto i = 1; i < sz; i++) {
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
    } else if (s == D_ForwardSlash) {
      if (c == '/') {
        // line comment
        decode_state_ = D_SingleLineComment;
        is_prev_back_slash_ = false;
      } else if (c == '*') {
        decode_state_ = D_InlineComment;
      } else {
        code_points_.push_back('/');
        decode_state_ = D_None;
        ret = decode(c);
      }
    } else if (s == D_BackSlash) {
      if (c == LF) {
        // line splicing
      } else if (c == 'u') {
        ASSERT(buffer_.empty(), "buffer must be empty");
        decode_state_ = D_LittleU;
      } else if (c == 'U') {
        ASSERT(buffer_.empty(), "buffer must be empty");
        decode_state_ = D_LargeU;
      } else {
        code_points_.push_back('\\');
        decode_state_ = D_None;
        ret = decode(c);
      }
    } else if (s == D_LittleU || s == D_LargeU) {
      // universal character name decoding
      if (isHex(c)) {
        buffer_.push_back(c);
        if (s == D_LittleU && buffer_.size() == 4) {
          int cp = toCodePoint(buffer_);
          code_points_.push_back(cp);
          buffer_.clear();
          ret = true;
        } else if (s == D_LargeU && buffer_.size() == 8) {
          // TODO: complete
          buffer_.clear();
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
    } else if (s == D_MayBeTriGraph1) {
      // tri-graph decoding
      if (c == '?') {
        decode_state_ = D_MayBeTriGraph2;
      } else {
        code_points_.push_back('?');
        decode_state_ = D_None;
        decode(c);
        ret = true;
      }
    } else if (s == D_MayBeTriGraph2) {
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
          cp = ']';
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
          code_points_.push_back('?');
          code_points_.push_back('?');
          decode_state_ = D_None;
          decode(c);
          ret = true;
          break;
      }
      if (cp != -1) {
        code_points_.push_back(cp);
        decode_state_ = D_None;
        ret = true;
      }

    } else if (s == D_SingleLineComment) {
      if (c == LF) {
        if (is_prev_back_slash_) {
          is_prev_back_slash_ = false;
        } else {
          decode_state_ = D_None;
        }
      } else if (c == '\\') {
        is_prev_back_slash_ = true;
      } else {
        is_prev_back_slash_ = false;
      }
    } else if (s == D_InlineComment) {
      if (c == '*') {
        decode_state_ = D_MayEndInlineComment;
      }
    } else if (s == D_MayEndInlineComment) {
      if (c == '/') {
        decode_state_ = D_None;
      } else {
        decode_state_ = D_InlineComment;
      }
    } else {
      ASSERT(false, "invalid statement");
    }


    // file terminating line-ending

    if (!ret && !code_points_.empty()) {
      ret = true;
    }
    return ret;
  }

  void emit(int c, bool cont) {

    if (pending_tokens.size() == 1 && state_ == S_Id) {
      switch (pending_tokens.front().type) {
        case Tk_CharacterLiteral:
          output.emit_user_defined_character_literal(pending_tokens.front().text + data_);
        case Tk_StringLiteral:
          output.emit_user_defined_string_literal(pending_tokens.front().text + data_);
        default:
          // TODO: complete
          break;
      }

      pending_tokens.clear();
      return;
    }

    if (!pending_tokens.empty()) {
      // TODO: complete
    }


    switch (state_) {
      case S_Id:
        output.emit_identifier(data_);
        break;
      case S_StringLiteral:
        output.emit_string_literal(data_);
        break;
    }

    state_ = S_None;
    saved_data_ = std::move(data_);
    if (cont) {
      process_None(c);
    }
  }

  void emit_error(const char *msg) {
    cerr << "ERROR: " << msg << endl;
    exit(EXIT_FAILURE);
  }

  void process_None(int c) {

    ASSERT(data_.empty(), "buffer must be empty");

    if (std::isspace(c) && c != 0x0A) {
      if (!is_prev_whitespace) {
        output.emit_whitespace_sequence();
        is_prev_whitespace = true;
      }
      return;
    }
    is_prev_whitespace = false;

    if (c == EndOfFile) {
      output.emit_eof();
    } else if (c == 0x0A) {
      if (data_.empty() || data_.back() != '\\') {
        output.emit_new_line();
      }
    } else if (c == '<' || c == '"') {
      // header-name
      state_ = S_HeaderName;
      data_.push_back(c);

      if (saved_data_ != "include") {
        state_ = S_StringLiteral;
      }
    } else if ('.' == c || std::isdigit(c)) {
      // pp-number
      state_ = S_PPNumber;
      data_.push_back(c);
    } else if (std::isalpha(c) || '_' == c) {
      state_ = S_Id;
      data_.push_back(c);
    }
  }

  void process_Identifier(int c) {
    if (std::isalnum(c) || '_' == c) {
      data_.push_back(c);
    } else if (c == '\'') {
      ASSERT(!data_.empty(), "buffer length must greater than 0");
      if (isEncodingPrefix(data_)) {
        state_ = S_CharacterLiteral;
        data_.push_back(c);
      } else {
        emit(c, true);
      }
    } else if (c == '"' || c == 'R') {
      ASSERT(expected_char_ == '\0', "expected character must be zero");
      if (isEncodingPrefix(data_)) {
        state_ = S_StringLiteral;
        data_.push_back(c);
        if (c == 'R') {
          expected_char_ = '"';
        }
      } else {
        emit(c, true);
      }
    } else {
      emit(c, true);
    }
  }

  void process_HeaderName(int c) {

    ASSERT(!data_.empty(), "buffer length must greater than 0");

    if (c == 0x0A || c == data_[0]) {
      emit(c, false);
    } else {
      data_.push_back(c);
    }
  }

  void process_ppnumber(int c) {

  }

  void process_character_literal(int c) {


    if (c == '\'') {
      data_.push_back(c);
      emit(c, false);
      return;
    }
  }

  void process_string_literal(int c) {

    if (is_raw_string_) {
      process_raw_string_literal(c);
    } else {
      process_normal_string_literal(c);
    }
  }

  void process_normal_string_literal(int c) {

    switch (string_inner_state_) {
      case Inner_None:
        if (c == 0x0A) {
          emit_error(kMissingStringTerminator);
        } else if (c == '\\') {
          string_inner_state_ = Inner_BackSlash;
        } else if (c == '\"') {
          data_.push_back(c);
          emit(c, false);
        } else {
          data_.push_back(c);
        }
        break;
      case Inner_BackSlash:
        ASSERT(c != 0x0A, "line ending character after back slash character cannot reach here");
        if (SimpleEscapeSequence_CodePoints.find(c) != SimpleEscapeSequence_CodePoints.end()) {
          data_.push_back(static_cast<char>(SimpleEscapeSequence_Map.at(c)));
        } else if (c == 'u' || c == 'U') {
          if (c == 'u') {
            string_inner_state_ = Inner_Little_U;
          } else {
            string_inner_state_ = Inner_Large_U;
          }
          escaped_value_.clear();
          hex_counts_ = 0;
        } else if (c == 'x') {
          string_inner_state_ = Inner_Hex;
          escaped_value_.clear();
        } else if (c >= '0' && c <= '7') {
          string_inner_state_ = Inner_Oct1;
          escaped_value_.clear();
          escaped_value_.push_back(c);
        } else {
          data_.push_back(c);
        }
        break;
      case Inner_Oct1:
        if (c >= '0' && c <= '7') {
          string_inner_state_ = Inner_Oct2;
          escaped_value_.push_back(c);
        } else {
          string_inner_state_ = Inner_None;
          char b = static_cast<char>(str2int(escaped_value_, 8));
          escaped_value_.clear();
          data_.push_back(b);
        }
        break;
      case Inner_Oct2: {
        bool cont = false;
        if (c >= '0' && c <= '7') {
          escaped_value_.push_back(c);
        } else {
          cont = true;
        }
        char b = static_cast<char>(str2int(escaped_value_, 8));
        escaped_value_.clear();
        data_.push_back(b);

        string_inner_state_ = Inner_None;
        if (cont) {
          process_normal_string_literal(c);
        }
      }
        break;

      case Inner_Hex:
        if (isHex(c)) {
          escaped_value_.push_back(c);
        } else {
          char h = static_cast<char>(str2int(escaped_value_, 16));
          escaped_value_.clear();
          data_.push_back(h);

          string_inner_state_ = Inner_None;
          process_normal_string_literal(c);
        }
        break;
      case Inner_Little_U:
        if (isHex(c)) {
          escaped_value_.push_back(c);
          ++hex_counts_;
          if (hex_counts_ == 4) {
            data_.append(littleU2Utf8(escaped_value_));
            string_inner_state_ = Inner_None;
          }
        } else {
          emit_error(kInvalidEscapeSequence);
        }

        break;
      case Inner_Large_U:
        if (isHex(c)) {
          escaped_value_.push_back(c);
          ++hex_counts_;
          if (hex_counts_ == 8) {
            data_.append(largeU2Utf8(escaped_value_));
            string_inner_state_ = Inner_None;
          } else {
            emit_error(kInvalidEscapeSequence);
          }
        }
        break;
      default:
        ASSERT(false, "invalid inner state");
        break;
    }
  }

  void process_raw_string_literal(int c) {

  }

private:
  string data_;
  string saved_data_;

  string buffer_;
  int counts_;
  DecodeState decode_state_;
  int code_point_;
  deque<int> code_points_;
  bool is_prev_back_slash_;

  TokenType prev_token_type_;
  StringState string_inner_state_;

  char expected_char_;
  string escaped_value_;
  int hex_counts_;

  bool is_raw_string_;
  bool is_escape_;
  bool is_prev_whitespace;

  vector<Token> pending_tokens;

  State state_;
};

int main() {

  //freopen("/home/syl/git/myproject/cppgm/pa1/tests/100-universal-character-name.t", "r", stdin);
  freopen("/tmp/c/input", "r", stdin);

  try {
    ostringstream oss;
    oss << cin.rdbuf();

    string input = oss.str();

    DebugPPTokenStream output;

    PPTokenizer tokenizer(output);

    for (char c : input) {
      unsigned char code_unit = c;
      tokenizer.process(code_unit);
    }

    tokenizer.process(EndOfFile);
  }
  catch (exception &e) {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
  }
}

