#ifndef FRONT_END_H
#define FRONT_END_H

#include "DataBuffer.h"

const size_t MAX_LANG_COMMAND_LEN        = 250;
const size_t START_LINE_ARRAY_LEN        = 300;

enum LangTokenType {
    NO_TYPE,
    NUM,
    VAR,
    ID,
    KEY_WORD,
    OP
};

union LangTokenData {
    double   num;
    OpCodes  op;
    char*    text;
    int      var;
    KeyWords K_word;
};

struct LangToken {
    LangTokenData data;
    LangTokenType type;
    size_t        position;
};

struct LangTokenArray {
    LangToken* Array;
    size_t     Pointer;
    size_t     size;
    size_t*    line_beginings;
    size_t     line_count;
    size_t     line_array_size;
};

#include "Lexer.h"
#include "NameTable.h"

langErrorCode lang_parser(const char* filename, TreeData* tree, LangNameTableArray* table_array);

#endif