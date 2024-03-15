#ifndef BACK_END_H
#define BACK_END_H

const size_t RAM_SIZE               = 100;
const size_t START_WHILE_STACK_SIZE = 20;

struct memoryTable {
    size_t Var[RAM_SIZE];   // В качестве значений - условные номера переменных, а в качестве адреса в массиве - смещение относительно rpx
    size_t Pointer;         // Количество элементов в массиве
};

langErrorCode lang_compiler(TreeData* tree, LangNameTableArray* table_array, FILE* asm_file);

#endif