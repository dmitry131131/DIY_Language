#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Tree.h"
#include "LangErrors.h"
#include "DataBuffer.h"
#include "NameTable.h"

static langErrorCode name_table_realloc(LangNameTable* name_table);

static langErrorCode write_dot_header(outputBuffer* buffer);
static langErrorCode write_dot_body(outputBuffer* buffer, const LangNameTableArray* table_array);
static langErrorCode write_dot_footer(outputBuffer* buffer);
static langErrorCode get_name_type(char (*string)[MAX_NAME_LEN], LangNameType type);
static langErrorCode write_func_table(outputBuffer* buffer, const LangNameTable* table);

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

    table_array->Array = (LangNameTable*) calloc(START_NAME_TABLE_ARRAY_SIZE, sizeof(LangNameTable));
    if (!table_array->Array)
    {
        return NAME_TABLE_ALLOC_MEMORY_ERROR;
    }

    table_array->Pointer  = 0;
    table_array->size     = 0;
    table_array->capacity = START_NAME_TABLE_ARRAY_SIZE;

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

    for (size_t i = 0; i <= table_array->size; i++)
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

langErrorCode add_to_name_table(LangNameTable* name_table, char** name, size_t number, LangNameType type)
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
    
    strncpy(name_table->Table[name_table->Pointer].name, *name, MAX_NAME_LEN);
    name_table->Table[name_table->Pointer].number = number;
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

const char* find_in_name_table_by_code(const LangNameTable* name_table, size_t number)
{
    assert(name_table);

    for (size_t i = 0; i <= name_table->Pointer; i++)
    {
        if (number == name_table->Table[i].number)
        {
            return name_table->Table[i].name;
        }
    }

    return nullptr;
}

LangNameTable* find_name_table(const LangNameTableArray* table_array, size_t number)
{
    assert(table_array);

    for (size_t i = 0; i < table_array->size; i++)
    {
        if (table_array->Array[i].table_number == number)
        {
            return &(table_array->Array[i]);
        }
    }

    return nullptr;
}

//#################################################################################################//
//-------------------------------------> Dump functions <------------------------------------------//
//#################################################################################################//

langErrorCode name_table_array_dump(const LangNameTableArray* table_array)
{
    assert(table_array);

    #define RETURN() do{                        \
        fclose(buffer.filePointer);             \
        return NAME_TABLE_DUMP_ERROR;           \
    }while(0)
    
    outputBuffer buffer = {};
    buffer.AUTO_FLUSH = true;

    system("mkdir -p temp");

    if (create_output_file(&(buffer.filePointer), "temp/table_dump.dot", TEXT))
    {
        RETURN();
    }

    if (write_dot_header(&buffer))
    {
        RETURN();
    }

    if (write_dot_body(&buffer, table_array))
    {
        RETURN();
    }

    if (write_dot_footer(&buffer))
    {
        RETURN();
    }

    if (write_buffer_to_file(&buffer))
    {
        RETURN();
    }

    fclose(buffer.filePointer);
    if (system("dot temp/table_dump.dot -Tpng -o table_dump.png"))
    {            
        return NAME_TABLE_DUMP_ERROR;  
    }

    #undef RETURN

    return NO_LANG_ERRORS;
}

static langErrorCode write_dot_header(outputBuffer* buffer)
{
    assert(buffer);
    print_to_buffer(buffer, "digraph G{\n"
                            "rankdir = TB;\n"
                            "bgcolor = \"#FFEFD5\";\n"
                            "node[color = \"#800000\", fontsize = 10];\n"
                            "edge[color = \"#800000\", fontsize = 15];\n\n");

    return NO_LANG_ERRORS;
}

static langErrorCode write_dot_body(outputBuffer* buffer, const LangNameTableArray* table_array)
{
    assert(buffer);
    assert(table_array);

    print_to_buffer(buffer, "0 [shape = Mrecord, style = filled, fillcolor = \"#FFF5EE\", color = \"#800000\", label = \" {№ | ID | TYPE | NAME} ");

    size_t current_number                     = 0;
    char*  current_name                       = nullptr;
    char   current_type[MAX_NAME_LEN]         = {};

    for (size_t i = 0; i < table_array->Array[0].Pointer; i++)
    {
        current_number = table_array->Array[0].Table[i].number;
        current_name   = table_array->Array[0].Table[i].name;
        get_name_type(&current_type, table_array->Array[0].Table[i].type);
        print_to_buffer(buffer, "| { %lu | %lu | %s | <f%lu> %s} ", i, current_number, current_type, current_number, current_name);
    }
    print_to_buffer(buffer, "\"];\n\n");

    for (size_t i = 1; i <= table_array->size; i++)
    {
        write_func_table(buffer, &(table_array->Array[i]));
    }

    return NO_LANG_ERRORS;
}

static langErrorCode write_func_table(outputBuffer* buffer, const LangNameTable* table)
{
    assert(buffer);
    assert(table);

    print_to_buffer(buffer, "%lu [shape = Mrecord, style = filled, fillcolor = \"#FFF5EE\", color = \"#800000\", label = \" {№ | ID | TYPE | NAME} ",
    table->table_number);

    size_t current_number                     = 0;
    char*  current_name                       = nullptr;
    char   current_type[MAX_NAME_LEN]         = {};

    for (size_t i = 0; i < table->Pointer; i++)
    {
        current_number = table->Table[i].number;
        current_name   = table->Table[i].name;
        get_name_type(&current_type, table->Table[i].type);
        print_to_buffer(buffer, "| { %lu | %lu | %s | %s} ", i, current_number, current_type, current_name);
    }

    print_to_buffer(buffer, "\"];\n"
                            "0:<f%lu> -> %lu [weight = 1, color = \"#0000ff\"];\n\n", table->table_number, table->table_number);


    return NO_LANG_ERRORS;
}

static langErrorCode get_name_type(char (*string)[MAX_NAME_LEN], LangNameType type)
{
    switch (type)
    {
    case NO_LANG_TYPE:
        strncpy(*string, "ERROR!", MAX_NAME_LEN);
        break;
    case VARIABLE:
        strncpy(*string, "Var", MAX_NAME_LEN);
        break;
    case FUNCTION:
        strncpy(*string, "Func", MAX_NAME_LEN);
        break;
    
    default:
        strncpy(*string, "ERROR!", MAX_NAME_LEN);
        break;
    }

    return NO_LANG_ERRORS;
}

static langErrorCode write_dot_footer(outputBuffer* buffer)
{  
    assert(buffer);
    print_to_buffer(buffer, "}");
    return NO_LANG_ERRORS;
}

