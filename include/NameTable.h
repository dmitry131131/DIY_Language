#ifndef NAME_TABLE_H
#define NAME_TABLE_H

const size_t MAX_NAME_LEN                = 250;
const size_t START_NAME_TABLE_SIZE       = 20;
const size_t START_NAME_TABLE_ARRAY_SIZE = 10;


enum LangNameType {
    NO_LANG_TYPE,
    VARIABLE,
    FUNCTION
};

struct LangNameTableUnit {
    char         name[MAX_NAME_LEN];
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

size_t find_in_name_table(const LangNameTable* name_table, const char* const* name);
langErrorCode add_to_name_table(LangNameTable* name_table, char** name, size_t number, LangNameType type);
langErrorCode name_table_array_dtor(LangNameTableArray* table_array);
langErrorCode name_table_array_ctor(LangNameTableArray* table_array);
langErrorCode name_table_dtor(LangNameTable* name_table);
langErrorCode name_table_ctor(LangNameTable* name_table);

langErrorCode name_table_array_dump(const LangNameTableArray* table_array);

#endif