#ifndef FRONT_END_H
#define FRONT_END_H

#include "DataBuffer.h"

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
    char         name[MAX_LANG_COMMAND_LEN];
    size_t       number;
    LangNameType type;
};

struct LangNameTable {
    LangNameTableUnit* Table;
    size_t table_number;
    size_t Pointer;
    size_t size;
};

struct LangNameTableArray 
{
    LangNameTable* Array;
    size_t Pointer;             // Указатель pointer указывает на текущую таблицу имён, в которую нужно записывать(брать) имена переменных
    size_t size;
    size_t capacity;
};

#include "Lexer.h"
#include "NameTable.h"

langErrorCode lang_parser(const char* filename, TreeData* tree, LangNameTableArray* table_array);

#endif