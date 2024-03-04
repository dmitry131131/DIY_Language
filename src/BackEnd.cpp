#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <sys/stat.h>

#include "Tree.h"
#include "LangErrors.h"
#include "DataBuffer.h"
#include "NameTable.h"
#include "BackEnd.h"

/*
    Описание работы стекового кадра: 
    В регистре rpx хранится адрес начала текущего стекового кадра. При вызове функции в стек пушится rpx, а затем в rpx кладётся текущий адрес.
    Теперь это начало нового стекового кадра. При этом адресация к переменным осуществляется по смещению от rpx. 
*/

static langErrorCode lang_compiler_recursive(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);
//--------------------------------------------------------------------------------------
static langErrorCode compile_function_definition(const TreeSegment* segment, const LangNameTableArray* table_array, outputBuffer* buffer);
static langErrorCode compile_parameters_in_definition(const TreeSegment* segment, const LangNameTable* name_table, memoryTable* RAM_Table, outputBuffer* buffer);
static langErrorCode compile_function_call(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);
static langErrorCode compile_parameters_in_call(const TreeSegment* segment, const LangNameTable* name_table, memoryTable* RAM_Table, outputBuffer* buffer);
//-------------------------------------------------------------------------------------

static langErrorCode compile_keyword(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);
static langErrorCode compile_var_declaration(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);
static langErrorCode compile_identifier(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);

static size_t find_in_RAM_table(const memoryTable* RAM_Table, size_t number);
static langErrorCode add_to_RAM_Table(memoryTable* RAM_Table, size_t number);

langErrorCode lang_compiler(TreeData* tree, LangNameTableArray* table_array, FILE* asm_file)
{
    assert(tree);
    assert(table_array);
    assert(asm_file);

    langErrorCode error = NO_LANG_ERRORS;

    memoryTable Global_RAM_Table = {};                  // Инициализация глобальной таблицы имён

    outputBuffer buffer = {.filePointer = asm_file, .AUTO_FLUSH = true};

    print_to_buffer(&buffer, "jmp main         ; Jump to start point\n"
                             "push rpx         ; save old rpx value\n");        // Вход в программу

    if ((error = lang_compiler_recursive(tree->root, table_array, &Global_RAM_Table, &buffer)))
    {
        fclose(asm_file);
        return error;
    }

    print_to_buffer(&buffer, "hlt               ; End of programm\n");        // Завершение программы
    
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
        if ((error = compile_function_definition(segment, table_array, buffer)))
        {
            return error;
        }
        break;
    // TODO Написать функцию декларации переменных
    case VAR_DECLARATION:
        if ((error = compile_var_declaration(segment, table_array, RAM_Table, buffer)))
        {
            return error;
        }
        break;

    case CALL:
        if ((error = compile_function_call(segment, table_array, RAM_Table, buffer)))
        {
            return error;
        }
        break;

    case KEYWORD:
        if ((error = compile_keyword(segment, table_array, RAM_Table, buffer)))
        {
            return error;
        }
        break;

    case DOUBLE_SEGMENT_DATA:
        print_to_buffer(buffer, "push %d\n", (int) segment->data.D_number);
        break;

    case IDENTIFIER:
        if ((error = compile_identifier(segment, table_array, RAM_Table, buffer)))
        {
            return error;
        }
        break;
    
    default:
        break;
    }

    return error;
}

static langErrorCode compile_identifier(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(table_array);
    assert(RAM_Table);
    assert(buffer);

    size_t var_offset = find_in_RAM_table(RAM_Table, segment->data.Id);

    if (var_offset == (size_t) -1 )
    {
        return NO_DECLARATION;
    }

    print_to_buffer(buffer, "push [rpx+%lu]     ; Variable with name: \"%s\"\n", var_offset,
                    find_in_name_table_by_code(&(table_array->Array[0]), segment->data.Id));

    return NO_LANG_ERRORS;
}

static langErrorCode compile_function_call(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(table_array);
    assert(RAM_Table);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    print_to_buffer(buffer, "; Saving memory stack pointer\n",
                            "push rpx\n");

    if ((error = compile_parameters_in_call(segment, &(table_array->Array[0]), RAM_Table, buffer)))        // BUG Сделать передачу правильной таблицы имён
    {   
        return error;
    }

    return error;
}

static langErrorCode compile_parameters_in_call(const TreeSegment* segment, const LangNameTable* name_table, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(name_table);
    assert(RAM_Table);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS; // BUG параметры должны быть уже определены и находиться в RAM_Table

    print_to_buffer(buffer, "; Parameter with name: %s\n", find_in_name_table_by_code(name_table, segment->left->left->data.Id));
    print_to_buffer(buffer, "push [rpx+%lu]\n", RAM_Table->Pointer);            //BUG адресация к переменным

    (RAM_Table->Pointer)++;

    if (segment->right)
    {
        if ((error = compile_parameters_in_call(segment->right, name_table, RAM_Table, buffer)))
        {
            return error;
        }
    }

    return error;
}

static langErrorCode compile_function_definition(const TreeSegment* segment, const LangNameTableArray* table_array, outputBuffer* buffer)
{
    assert(segment);
    assert(table_array);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    memoryTable RAM_Table = {};                         // Инициализация локальной таблицы

    const char* function_name = find_in_name_table_by_code(&(table_array->Array[0]), segment->data.Id);

    print_to_buffer(buffer, "; Function declaration with name: %s\n", function_name);

    if (!strcmp("главная", function_name))
    {
        print_to_buffer(buffer, ":main\n");
    }

    const TreeSegment* parameter_segment = segment->right;

    if (parameter_segment->left)
    {
        print_to_buffer(buffer, "; Accepting parameners\n");
        if ((error = compile_parameters_in_definition(parameter_segment->left, find_name_table(table_array, segment->data.Id), &RAM_Table, buffer)))
        {
            return error;
        }
    }

    if ((error = lang_compiler_recursive(parameter_segment->right, table_array, &RAM_Table, buffer)))
    {
        return error;
    }

    print_to_buffer(buffer, "pop rpx\n"); // Восстановление указателя на стековый кадр после обработки функции

    return error;
}

static langErrorCode compile_parameters_in_definition(const TreeSegment* segment, const LangNameTable* name_table, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(name_table);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    if ((error = add_to_RAM_Table(RAM_Table, segment->left->left->data.Id)))
    {
        return error;
    }
    print_to_buffer(buffer, "; Parameter declaration with name: %s\n", find_in_name_table_by_code(name_table, segment->left->left->data.Id));
    print_to_buffer(buffer, "pop [rpx+%lu]\n", RAM_Table->Pointer);

    if ((error = add_to_RAM_Table(RAM_Table, segment->left->left->data.Id)))
    {
        return error;
    }

    if (segment->right)
    {
        if ((error = compile_parameters_in_definition(segment->right, name_table, RAM_Table, buffer)))
        {
            return error;
        }
    }

    return error;
}

static langErrorCode compile_var_declaration(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(table_array);
    assert(RAM_Table);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    if ((error = add_to_RAM_Table(RAM_Table, segment->data.Id)))
    {
        return error;
    }

    if (segment->right->type == IDENTIFIER)
    {
        print_to_buffer(buffer, "; Defined variable with name: %s\n", find_in_name_table_by_code(&(table_array->Array[0]), segment->right->data.Id));
        return error;
    }

    if ((error = lang_compiler_recursive(segment->right, table_array, RAM_Table, buffer)))
    {
        return error;
    }

    return error;
}

#define RECURSE_LEFT_BRANCH do{                                                                     \
    if (segment->left)                                                                              \
    {                                                                                               \
        if ((error = lang_compiler_recursive(segment->left, table_array, RAM_Table, buffer)))       \
        {                                                                                           \
            return error;                                                                           \
        }                                                                                           \
    }                                                                                               \
}while (0)

#define RECURSE_RIGHT_BRANCH do{                                                                    \
    if (segment->right)                                                                             \
    {                                                                                               \
        if ((error = lang_compiler_recursive(segment->right, table_array, RAM_Table, buffer)))      \
        {                                                                                           \
            return error;                                                                           \
        }                                                                                           \
    }                                                                                               \
}while(0)  

static langErrorCode compile_keyword(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(table_array);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    switch ((size_t) segment->data.K_word)
    {
    case KEY_NEXT:
        RECURSE_LEFT_BRANCH;
        RECURSE_RIGHT_BRANCH;   
        break;
    case KEY_OUT:
        RECURSE_RIGHT_BRANCH;

        print_to_buffer(buffer, "out        ; Print function\n");
        break;

    case KEY_ASSIGMENT:

        RECURSE_LEFT_BRANCH;
        print_to_buffer(buffer, "pop [rpx+%lu]\n", find_in_RAM_table(RAM_Table, segment->right->data.Id));

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
        if (number == RAM_Table->Var[i])
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

    RAM_Table->Var[RAM_Table->Pointer] = number;
    (RAM_Table->Pointer)++;

    return NO_LANG_ERRORS;
}

