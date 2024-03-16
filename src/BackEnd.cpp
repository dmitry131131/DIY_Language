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
#include "Stack.h"

/*
    Описание работы стекового кадра: 
    В регистре rpx хранится адрес начала текущего стекового кадра. При вызове функции в стек пушится rpx, а затем в rpx кладётся текущий адрес.
    Теперь это начало нового стекового кадра. При этом адресация к переменным осуществляется по смещению от rpx. 
*/
static size_t scope_counter    = 0;
static size_t last_while_scope = 0;
static struct Stack while_stack = {};

static langErrorCode lang_compiler_recursive(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);
//--------------------------------------------------------------------------------------
static langErrorCode compile_function_definition(const TreeSegment* segment, const LangNameTableArray* table_array, outputBuffer* buffer);
static langErrorCode compile_parameters_in_definition(const TreeSegment* segment, const LangNameTable* name_table, memoryTable* RAM_Table, outputBuffer* buffer);
static langErrorCode compile_function_call(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);
static langErrorCode compile_parameters_in_call(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer);
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

    STACK_CTOR(&while_stack, START_WHILE_STACK_SIZE);

    outputBuffer buffer = {.filePointer = asm_file, .AUTO_FLUSH = true};

    print_to_buffer(&buffer, "push rpx          ; save old rpx value\n"
                             "call main         ; Jump to start point\n");        // Вход в программу

    print_to_buffer(&buffer, "hlt               ; End of programm\n");        // Завершение программы

    if ((error = lang_compiler_recursive(tree->root, table_array, &Global_RAM_Table, &buffer)))
    {
        fclose(asm_file);
        return error;
    }
    
    write_buffer_to_file(&buffer);

    STACK_DTOR(&while_stack);
    
    return error;
}

// TODO Написать подробные коментарии в ассемблерный файл(что из чего получено и как)

// TODO Написать проверку кол-ва аргументов переданных в функцию с максимальным числом аргкментов
// TODO Написать проверку вызова несуществующих функций
// Для этого нужно будет написать функцию прохода по массиву лексемм, с целью занесания в таблицу имён большей информации о функциях(кол-во  аргументов) 
// TODO Обработка глобальных переменных

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

    if (segment->left)
    {
        if ((error = compile_parameters_in_call(segment->left, table_array, RAM_Table, buffer)))    
        {   
            return error;
        }
    }

    print_to_buffer(buffer, "; Saving memory stack pointer\n"
                            "push rpx       ; Save old stack pointr\n"
                            "push rpx\n"
                            "push %lu\n"
                            "add\n"
                            "pop rpx\n", RAM_Table->Pointer);

    print_to_buffer(buffer, "call Function_%lu\n", segment->right->data.Id);

    if (segment->parent->data.K_word != KEY_NEXT)
    {
        print_to_buffer(buffer, "push rax\n");
    }

    return error;
}

static langErrorCode compile_parameters_in_call(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(table_array);
    assert(RAM_Table);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS; // BUG параметры должны быть уже определены и находиться в RAM_Table
    print_to_buffer(buffer, "; Compile parameter\n");
    if ((error = lang_compiler_recursive(segment->left, table_array, RAM_Table, buffer)))
    {
        return error;
    }

    if (segment->right)
    {
        if ((error = compile_parameters_in_call(segment->right, table_array, RAM_Table, buffer)))
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

    print_to_buffer(buffer, "\n\n; Function declaration with name: %s\n", function_name);

    if (!strcmp("главная", function_name))
    {
        print_to_buffer(buffer, ":main\n");
    }
    else
    {
        print_to_buffer(buffer, ":Function_%lu\n", segment->data.Id);
    }

    const TreeSegment* parameter_segment = segment->right;

    if (parameter_segment->left)
    {
        print_to_buffer(buffer, "; Save old rpx value in rbx\n"
                                "pop rbx\n"
                                "; Accepting parameners\n");
        if ((error = compile_parameters_in_definition(parameter_segment->left, find_name_table(table_array, segment->right->data.Id), &RAM_Table, buffer)))
        {
            return error;
        }
        print_to_buffer(buffer, "; Repair old rpx value to stack\n"
                                "push rbx\n");
    }

    if ((error = lang_compiler_recursive(parameter_segment->right, table_array, &RAM_Table, buffer)))
    {
        return error;
    }

    print_to_buffer(buffer, "pop rpx\n"
                            "ret\n"); // Восстановление указателя на стековый кадр после обработки функции

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
    print_to_buffer(buffer, "pop [rpx+%lu]\n", RAM_Table->Pointer - 1);

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

#define ADD_ZERO_BRANCH_COMMAND(_KEY_WORD_, _COMENT_TEXT_, _COMMAND_TEXT_)                          \
case _KEY_WORD_:                                                                                    \
    print_to_buffer(buffer, _COMENT_TEXT_);                                                         \
                                                                                                    \
    _COMMAND_TEXT_                                                                                  \
    break;                                                                                          \

#define ADD_RIGHT_BRANCH_COMMAND(_KEY_WORD_, _COMENT_TEXT_, _COMMAND_TEXT_)                         \
case _KEY_WORD_:                                                                                    \
    print_to_buffer(buffer, _COMENT_TEXT_);                                                         \
    RECURSE_RIGHT_BRANCH;                                                                           \
                                                                                                    \
    _COMMAND_TEXT_                                                                                  \
    break;                                                                                          \

#define ADD_DOUBLE_BRANCH_COMMAND(_KEY_WORD_, _COMENT_TEXT_, _COMMAND_TEXT_)                        \
    case _KEY_WORD_:                                                                                \
    print_to_buffer(buffer, _COMENT_TEXT_);                                                         \
    RECURSE_LEFT_BRANCH;                                                                            \
    RECURSE_RIGHT_BRANCH;                                                                           \
                                                                                                    \
    _COMMAND_TEXT_                                                                                  \
    break;                                                                                          \

#define ADD_LEFT_BRANCH_COMMAND(_KEY_WORD_, _COMENT_TEXT_, _COMMAND_TEXT_)                          \
    case _KEY_WORD_:                                                                                \
    print_to_buffer(buffer, _COMENT_TEXT_);                                                         \
    RECURSE_LEFT_BRANCH;                                                                            \
                                                                                                    \
    _COMMAND_TEXT_                                                                                  \
    break;                                                                                          \

static langErrorCode compile_keyword(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, outputBuffer* buffer)
{
    assert(segment);
    assert(table_array);
    assert(buffer);

    langErrorCode error = NO_LANG_ERRORS;

    size_t temp_scope_counter = 0;

    switch ((size_t) segment->data.K_word)
    {

    ADD_ZERO_BRANCH_COMMAND(KEY_IN,         "; Print in()\n", print_to_buffer(buffer, "in\n"););
    ADD_ZERO_BRANCH_COMMAND(KEY_BREAK,      "",               print_to_buffer(buffer, "jmp Skip_scope_%lu     ; Break\n", last_while_scope););
    ADD_ZERO_BRANCH_COMMAND(KEY_CONTINUE,   "",               print_to_buffer(buffer, "jmp While_next_%lu     ; Continue\n", last_while_scope););

    ADD_LEFT_BRANCH_COMMAND(KEY_ASSIGMENT, "",
                            print_to_buffer(buffer, "pop [rpx+%lu]\n", find_in_RAM_table(RAM_Table, segment->right->data.Id)););

    ADD_RIGHT_BRANCH_COMMAND(KEY_OUT,           "; Print function\n",  print_to_buffer(buffer, "out\n"););
    ADD_RIGHT_BRANCH_COMMAND(KEY_SIN,           "; Print sin\n",       print_to_buffer(buffer, "sin\n"););
    ADD_RIGHT_BRANCH_COMMAND(KEY_COS,           "; Print cos\n",       print_to_buffer(buffer, "cos\n"););
    ADD_RIGHT_BRANCH_COMMAND(KEY_RETURN,        "; Return\n",          print_to_buffer(buffer, "pop rax\n"
                                                                                               "pop rpx\n"
                                                                                               "ret\n"););
    
    ADD_DOUBLE_BRANCH_COMMAND(KEY_NEXT,         "",                    ;);
    ADD_DOUBLE_BRANCH_COMMAND(KEY_PLUS,         "; Print summ node\n", print_to_buffer(buffer, "add\n"););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_MINUS,        "; Print summ node\n", print_to_buffer(buffer, "sub\n"););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_MUL,          "; Print summ node\n", print_to_buffer(buffer, "mul\n"););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_DIV,          "; Print summ node\n", print_to_buffer(buffer, "div\n"););

    ADD_DOUBLE_BRANCH_COMMAND(KEY_MORE,         "",                    print_to_buffer(buffer, "jbe Skip_scope_%lu\n", scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_MORE_EQUAL,   "",                    print_to_buffer(buffer, "jb Skip_scope_%lu\n",  scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_LESS,         "",                    print_to_buffer(buffer, "jae Skip_scope_%lu\n", scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_LESS_EQUAL,   "",                    print_to_buffer(buffer, "ja Skip_scope_%lu\n",  scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_EQUAL,        "",                    print_to_buffer(buffer, "jne Skip_scope_%lu\n", scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_NOT_EQUAL,    "",                    print_to_buffer(buffer, "je Skip_scope_%lu\n",  scope_counter););

    case KEY_IF:
        print_to_buffer(buffer, "; If %lu section\n", scope_counter);
        temp_scope_counter = scope_counter;                             // save current scope counter
        RECURSE_LEFT_BRANCH;
        scope_counter++;
        RECURSE_RIGHT_BRANCH;
        print_to_buffer(buffer, ":Skip_scope_%lu        ; End of if %lu section\n\n", temp_scope_counter, temp_scope_counter);

        break;

    case KEY_WHILE:
        print_to_buffer(buffer, "; While %lu section\n"
                                ":While_next_%lu\n", scope_counter, scope_counter);
        STACK_PUSH(&while_stack, (elem_t) last_while_scope);               // save old last while scope number in stack
        last_while_scope   = scope_counter;                             // save last while scope number 
        temp_scope_counter = scope_counter;                             // save current scope counter
        RECURSE_LEFT_BRANCH;
        scope_counter++;
        RECURSE_RIGHT_BRANCH;
        last_while_scope = (size_t) STACK_POP(&while_stack);                     // repair last while scope number from stack
        print_to_buffer(buffer, "jmp While_next_%lu\n", temp_scope_counter);
        print_to_buffer(buffer, ":Skip_scope_%lu        ; End of if %lu section\n\n", temp_scope_counter, temp_scope_counter);

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

