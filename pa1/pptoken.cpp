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

};

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

// Tokenizer
struct PPTokenizer {
  IPPTokenStream &output;


  PPTokenizer(IPPTokenStream &output)
      : output(output) {
    state_ = S_None;
    prev_token_type_ = Tk_Null;
  }

  void process(int c) {

    // 1. do translation features
    // 2. tokenize resulting stream
    // 3. call an output.emit_* function for each token.
    switch (state_) {
      case S_None:
        process_None(c);
        break;
      case S_Id:
        process_Identifier(c);
        break;
      case S_HeaderName:
        process_ppnumber(c);
        break;
      case S_CharacterLiteral:
        process_character_literal(c);
        break;
      case S_StringLiteral:
        process_string_literal(c);
        break;
      default:
        break;
    }

    // TIP: Reference implementation is about 1000 lines of code.
    // It is a state machine with about 50 states, most of which
    // are simple transitions of the operators.

  }

private:
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
    }

    state_ = S_None;
    saved_data_ = std::move(data_);
    if (cont) {
      process_None(c);
    }
  }

  void process_None(int c) {

    ASSERT(data_.empty(), "buffer must be empty");

    if (c == EndOfFile) {
      output.emit_eof();
    } else if (c == 0xA0) {
      if (!data_.empty() && data_.back() == '\\') {

      } else {

      }
    } else if (c == '<' || c == '"') {
      // header-name
      state_ = S_HeaderName;
      data_.push_back(c);
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
      if (startsWith(data_, "u8") || data_[0] == 'u' || data_[0] == 'U' || data_[0] == 'L') {
        state_ = S_CharacterLiteral;
        data_.push_back(c);
      } else {
        emit(c, true);
      }
    } else {
      emit(c, true);
    }
  }

  void process_HeaderName(int c) {

    ASSERT(!data_.empty(), "buffer length must greater than 0");

    if (c == 0xA0 || c == data_[0]) {
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

  }
private:
  string data_;
  string saved_data_;
  TokenType prev_token_type_;

  vector<Token> pending_tokens;

  State state_;
};

int main() {

  freopen("/home/syl/git/myproject/cppgm/pa1/tests/100-a.t", "r", stdin);

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

