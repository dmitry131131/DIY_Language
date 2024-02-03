#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <math.h>
#include <sys/stat.h>

#include "Tree.h"
#include "LangErrors.h"
#include "DataBuffer.h"
#include "NameTable.h"
#include "BackEnd.h"

static langErrorCode lang_compiler_recursive(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);
static langErrorCode compile_function_definition(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);
static langErrorCode compile_parameters_in_definition(const TreeSegment* segment, const LangNameTable* name_table, memoryTable* RAM_Table, outputBuffer* buffer);
static langErrorCode compile_keyword(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);

static size_t find_in_RAM_table(const memoryTable* RAM_Table, size_t number);
static langErrorCode add_to_RAM_Table(memoryTable* RAM_Table, size_t number);

langErrorCode lang_compiler(TreeData* tree, LangNameTableArray* table_array, FILE* asm_file)
{
    assert(tree);
    assert(table_array);
    assert(asm_file);

    langErrorCode error = NO_LANG_ERRORS;

    outputBuffer buffer = {.filePointer = asm_file, .AUTO_FLUSH = true};

    memoryTable RAM_Table = {};

    if ((error = lang_compiler_recursive(tree->root, table_array, &RAM_Table, &buffer)))
    {
        fclose(asm_file);
        return error;
    }
    
    write_buffer_to_file(&buffer);
    
    return error;
}

// TODO Написать отдельную функцию обработки keyword(некоторые узлы обрабатывать особым образом, например =) 
// TODO Написать подробные коментарии в ассемблерный файл(что из чего получено и как)
// TODO Написать отдельную функцию обработки деклараций функций
// TODO Придумать и описать систему передачи аргументов функции в функцию
// TODO Придумать и описать систему адресации в памяти

// Нельзя использоввать метки русскими буквами

static langErrorCode lang_compiler_recursive(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer)
{   
    assert(segment);
    assert(table_array);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    switch ((size_t) segment->type)
    {
    case FUNCTION_DEFINITION:
        if ((error = compile_function_definition(segment, table_array, RAM_Table, buffer)))
        break;
    // TODO Написать функцию декларации переменных
    case VAR_DECLARATION:

        break;

    case KEYWORD:
        if ((error = compile_keyword(segment, table_array, RAM_Table, buffer)))
        {
            return error;
        }
        break;
    
    default:
        break;
    }

    return error;
}

static langErrorCode compile_function_definition(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(table_array);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    print_to_buffer(buffer, "; Function declaration with name: %s\n", find_in_name_table_by_code(&(table_array->Array[0]), segment->data.Id));

    const TreeSegment* parameter_segment = segment->right;

    if (parameter_segment->left)
    {
        print_to_buffer(buffer, "; Accepting parameners\n");
        if ((error = compile_parameters_in_definition(parameter_segment->left, find_name_table(table_array, segment->data.Id), RAM_Table, buffer)))
        {
            return error;
        }
    }

    if ((error = lang_compiler_recursive(parameter_segment->right, table_array, RAM_Table, buffer)))
    {
        return error;
    }

    return error;
}
// BUG Вопрос по адресации между таблицами имён (возможно стоит в таблицу имён для функций вписывать номер таблицы имён в массиве таблиц)
static langErrorCode compile_parameters_in_definition(const TreeSegment* segment, const LangNameTable* name_table, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(name_table);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    // Ошибка повторной декларации, возможно её можно ловить ещё на стадии фронтенда
    if (find_in_RAM_table(RAM_Table, segment->left->left->data.Id) != (size_t) -1)
    {
        // error
    }

    if ((error = add_to_RAM_Table(RAM_Table, segment->left->left->data.Id)))
    {
        return error;
    }
    print_to_buffer(buffer, "; Parameter declaration with name: %s\n", find_in_name_table_by_code(name_table, segment->left->left->data.Id));
    print_to_buffer(buffer, "pop [%lu]\n", RAM_Table->Pointer - 1);

    if (segment->right)
    {
        if ((error = compile_parameters_in_definition(segment->right, name_table, RAM_Table, buffer)))
        {
            return error;
        }
    }

    return error;
}

static langErrorCode compile_keyword(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(table_array);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    switch ((size_t) segment->data.K_word)
    {
    case KEY_NEXT:
        if ((error = lang_compiler_recursive(segment->left, table_array, RAM_Table, buffer)))
        {
            return error;
        }
        break;
    
    default:
        break;
    }

    return error;
}

//#################################################################################################//
//-------------------------------------> RAM functions <-------------------------------------------//
//#################################################################################################//

static size_t find_in_RAM_table(const memoryTable* RAM_Table, size_t number)
{
    assert(RAM_Table);

    for (size_t i = 0; i < RAM_Table->Pointer; i++)
    {
        if (number == RAM_Table->RAM[i])
        {
            return i;
        }
    }

    return (size_t) -1;
}

static langErrorCode add_to_RAM_Table(memoryTable* RAM_Table, size_t number)
{
    if (RAM_Table->Pointer >= RAM_SIZE)
    {
        return PROCESSOR_MEMORY_OVER;
    }

    RAM_Table->RAM[RAM_Table->Pointer] = number;
    (RAM_Table->Pointer)++;

    return NO_LANG_ERRORS;
}