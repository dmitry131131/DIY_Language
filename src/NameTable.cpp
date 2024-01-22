#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Tree.h"
#include "LangErrors.h"
#include "FrontEnd.h"
#include "NameTable.h"

static langErrorCode name_table_realloc(LangNameTable* name_table);

langErrorCode name_table_ctor(LangNameTable* name_table)
{
    assert(name_table);
    langErrorCode error = NO_LANG_ERRORS;

    name_table->Table = (LangNameTableUnit*) calloc(START_NAME_TABLE_SIZE, sizeof(LangNameTableUnit));
    if (!name_table->Table)
    {
        error = NAME_TABLE_ALLOC_MEMORY_ERROR;
    }

    name_table->Pointer = 0;
    name_table->size = START_NAME_TABLE_SIZE;

    return error;
}

langErrorCode name_table_dtor(LangNameTable* name_table)
{
    assert(name_table);

    if (!name_table->Table)
    {
        return BAD_NAME_TABLE;
    }

    free(name_table->Table);
    name_table->Pointer = 0;
    name_table->size    = 0;

    return NO_LANG_ERRORS;
}

langErrorCode name_table_array_ctor(LangNameTableArray* table_array)
{
    assert(table_array);

    table_array->Array = (LangNameTable*) calloc(2, sizeof(LangNameTable));
    if (!table_array->Array)
    {
        return NAME_TABLE_ALLOC_MEMORY_ERROR;
    }

    table_array->Pointer = 0;
    table_array->size    = START_NAME_TABLE_ARRAY_SIZE;

    return NO_LANG_ERRORS;
}

langErrorCode name_table_array_dtor(LangNameTableArray* table_array)
{
    assert(table_array);
    langErrorCode error = NO_LANG_ERRORS;

    if (!table_array->Array)
    {
        return BAD_NAME_TABLE;
    }

    for (size_t i = 0; i <= table_array->Pointer; i++)
    {
        name_table_dtor(&(table_array->Array[i]));
    }

    free(table_array->Array);
    table_array->Pointer = 0;
    table_array->size    = 0;

    return error;
}

static langErrorCode name_table_realloc(LangNameTable* name_table)
{
    assert(name_table);
    langErrorCode error = NO_LANG_ERRORS;

    name_table->Table = (LangNameTableUnit*) realloc(name_table->Table, 2 * name_table->size * sizeof(LangNameTableUnit));
    if (!name_table->Table)
    {
        error = NAME_TABLE_ALLOC_MEMORY_ERROR;
    }

    name_table->size = 2 * name_table->size;

    return error;
}

langErrorCode add_to_name_table(LangNameTable* name_table, char** name, LangNameType type)
{
    assert(name_table);
    assert(name);
    langErrorCode error = NO_LANG_ERRORS;

    if (name_table->Pointer >= name_table->size)
    {
        if ((error = name_table_realloc(name_table)))
        {
            return error;
        }
    }
    printf("%s\n", *name);
    strncpy(name_table->Table[name_table->Pointer].name, *name, MAX_LANG_COMMAND_LEN);
    name_table->Table[name_table->Pointer].number = name_table->Pointer;
    name_table->Table[name_table->Pointer].type   = type;

    (name_table->Pointer)++;

    return error;
}

size_t find_in_name_table(const LangNameTable* name_table, const char* const* name)
{
    assert(name_table);
    assert(name);

    size_t position = 0;
    for (size_t i = 0; i < name_table->Pointer; i++)
    {
        if (!strcmp(*name, name_table->Table[i].name))
        {
            position = name_table->Table[i].number;
        }
    }

    return position;
}