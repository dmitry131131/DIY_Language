#ifndef BACK_END_H
#define BACK_END_H

const size_t RAM_SIZE = 100;

struct memoryTable {
    size_t RAM[RAM_SIZE];
    size_t Pointer;
};

langErrorCode lang_compiler(TreeData* tree, LangNameTableArray* table_array, FILE* asm_file);

#endif