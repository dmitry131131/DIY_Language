#ifndef NAME_TABLE_H
#define NAME_TABLE_H

size_t find_in_name_table(const LangNameTable* name_table, const char* const* name);
langErrorCode add_to_name_table(LangNameTable* name_table, char** name, LangNameType type);
langErrorCode name_table_array_dtor(LangNameTableArray* table_array);
langErrorCode name_table_array_ctor(LangNameTableArray* table_array);
langErrorCode name_table_dtor(LangNameTable* name_table);
langErrorCode name_table_ctor(LangNameTable* name_table);

#endif