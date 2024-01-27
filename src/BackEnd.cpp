#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "Tree.h"
#include "LangErrors.h"
#include "DataBuffer.h"
#include "NameTable.h"
#include "BackEnd.h"

langErrorCode lang_compiler(TreeData* tree, LangNameTableArray* table_array, FILE* asm_file)
{
    assert(tree);
    assert(table_array);
    assert(asm_file);

    outputBuffer buffer = {.filePointer = asm_file};
    buffer_ctor(&buffer, 10000);   // FIXME нормальный размер

    

    buffer_dtor(&buffer);
    return NO_LANG_ERRORS;
}