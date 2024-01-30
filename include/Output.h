#ifndef LANG_OUTPUT_H
#define LANG_OUTPUT_H

langErrorCode write_lang_tree_to_file(const char* filename, const TreeData* tree);

langErrorCode write_name_table_array_to_file(const char* filename, const LangNameTableArray* table_array);

#endif