#ifndef FRONT_END_H
#define FRONT_END_H

const size_t MAX_LANG_COMMAND_LEN        = 250;
const size_t START_NAME_TABLE_SIZE       = 20;
const size_t START_NAME_TABLE_ARRAY_SIZE = 10;

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

enum LangNameType {
    NO_LANG_TYPE,
    VARIABLE,
    FUNCTION
};

struct LangNameTableUnit {
    char*        name;
    int          number;
    LangNameType type;
};

struct LangNameTable {
    LangNameTableUnit* Table;
    size_t Pointer;
    size_t size;
};

struct LangNameTableArray 
{
    LangNameTable* Array;
    size_t Pointer;
    size_t size;
};


langErrorCode lang_parser(const char* filename, TreeData* tree, LangNameTableArray* table_array);

langErrorCode name_table_ctor(LangNameTable* name_table);
langErrorCode name_table_dtor(LangNameTable* name_table);

langErrorCode name_table_array_ctor(LangNameTableArray* table_array);
langErrorCode name_table_array_dtor(LangNameTableArray* table_array);

#endif