#ifndef FRONT_END_H
#define FRONT_END_H

const size_t MAX_LANG_COMMAND_LEN = 250;

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
};

langErrorCode lang_parser(const char* filename, TreeData* tree);



#endif