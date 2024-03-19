#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <sys/stat.h>

#include "Tree.h"
#include "LangErrors.h"
#include "DataBuffer.h"
#include "Stack.h"
#include "NameTable.h"
#include "BackEnd.h"

struct BackendContext {
    size_t       scope_counter;
    size_t       last_while_scope;
    size_t       current_function;
    memoryTable  global_RAM_Table;
    Stack        while_stack;
    outputBuffer buffer;
};

/*
    Описание работы стекового кадра: 
    В регистре rpx хранится адрес начала текущего стекового кадра. При вызове функции в стек пушится rpx, а затем в rpx кладётся текущий адрес.
    Теперь это начало нового стекового кадра. При этом адресация к переменным осуществляется по смещению от rpx. 
*/

// TODO Возможно загнать эти переменные и стек вместе с выходным буффером в единую структуру и передавать её всем функциям
// Туда же положить информацию о текущей функции, которая компилируется - это нужно для комментариев с именами функций и переменных

static langErrorCode lang_compiler_recursive(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context);
//--------------------------------------------------------------------------------------
static langErrorCode compile_function_definition(const TreeSegment* segment, const LangNameTableArray* table_array, BackendContext* context);
static langErrorCode compile_parameters_in_definition(const TreeSegment* segment, const LangNameTable* name_table, memoryTable* RAM_Table, BackendContext* context);
static langErrorCode compile_function_call(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context);
static langErrorCode compile_parameters_in_call(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context);
//-------------------------------------------------------------------------------------

static langErrorCode compile_keyword(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context);
static langErrorCode compile_var_declaration(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context);
static langErrorCode compile_global_var_declaration(const TreeSegment* segment, const LangNameTableArray* table_array, BackendContext* context);
static langErrorCode compile_identifier(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context);

static langErrorCode compile_global_variables(const TreeSegment* segment, LangNameTableArray* table_array, BackendContext* context);

static size_t find_in_RAM_table(const memoryTable* RAM_Table, size_t number);
static langErrorCode add_to_RAM_Table(memoryTable* RAM_Table, size_t number);

static langErrorCode BackendContext_ctor(BackendContext* context, FILE* asm_file);

langErrorCode lang_compiler(TreeData* tree, LangNameTableArray* table_array, FILE* asm_file)
{
    assert(tree);
    assert(table_array);
    assert(asm_file);

    langErrorCode error = NO_LANG_ERRORS;

    BackendContext context = {};

    if ((error = BackendContext_ctor(&context, asm_file)))
    {
        return error;
    }

    print_to_buffer(&(context.buffer), "; Compile global variables\n");

    if ((error = compile_global_variables(tree->root, table_array, &context)))
    {
        return error;
    }

    print_to_buffer(&(context.buffer),  "push %lu\n"
                                        "pop rpx\n"
                                        ";=======================================\n"
                                        "push rpx          ; save old rpx value\n"
                                        "call main         ; Jump to start point\n"
                                        "hlt               ; End of programm\n", context.global_RAM_Table.Pointer);        // Вход в программу

    if ((error = lang_compiler_recursive(tree->root, table_array, &(context.global_RAM_Table), &context)))
    {
        fclose(asm_file);
        return error;
    }
    
    write_buffer_to_file(&(context.buffer));

    STACK_DTOR(&(context.while_stack));
    
    return error;
}

static langErrorCode compile_global_variables(const TreeSegment* segment, LangNameTableArray* table_array, BackendContext* context)
{
    assert(segment);
    assert(table_array);
    assert(context);

    langErrorCode error = NO_LANG_ERRORS;

    if (segment->left->type == VAR_DECLARATION)
    {
        if ((error = compile_global_var_declaration(segment->left, table_array, context)))
        {
            return error;
        }
    }

    if (segment->right)
    {
        if ((error = compile_global_variables(segment->right, table_array, context)))
        {
            return error;
        }
    }

    return error;
}

// TODO Написать подробные коментарии в ассемблерный файл(что из чего получено и как)

// TODO Написать проверку кол-ва аргументов переданных в функцию с максимальным числом аргкментов
// TODO Написать проверку вызова несуществующих функций
// Для этого нужно будет написать функцию прохода по массиву лексемм, с целью занесания в таблицу имён большей информации о функциях(кол-во  аргументов) 
// TODO Обработка глобальных переменных

// Нельзя использоввать метки русскими буквами

static langErrorCode lang_compiler_recursive(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context)
{   
    assert(segment);
    assert(table_array);
    assert(context);

    langErrorCode error = NO_LANG_ERRORS;

    switch ((size_t) segment->type)
    {
    case FUNCTION_DEFINITION:
        if ((error = compile_function_definition(segment, table_array, context)))
        {
            return error;
        }
        break;
    
    case VAR_DECLARATION:
        if ((error = compile_var_declaration(segment, table_array, RAM_Table, context)))
        {
            return error;
        }
        break;
    
    case CALL:
        if ((error = compile_function_call(segment, table_array, RAM_Table, context)))
        {
            return error;
        }
        break;
    
    case KEYWORD:
        if ((error = compile_keyword(segment, table_array, RAM_Table, context)))
        {
            return error;
        }
        break;

    case DOUBLE_SEGMENT_DATA:
        print_to_buffer(&(context->buffer), "push %d\n", (int) segment->data.D_number);
        break;

    case IDENTIFIER:
        if ((error = compile_identifier(segment, table_array, RAM_Table, context)))
        {
            return error;
        }
        break;
    
    default:
        break;
    }

    return error;
}

static langErrorCode compile_identifier(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context)
{
    assert(segment);
    assert(table_array);
    assert(RAM_Table);
    assert(context);

    size_t var_offset = find_in_RAM_table(RAM_Table, segment->data.Id);

    if (var_offset == (size_t) -1)
    {
        var_offset = find_in_RAM_table(&(context->global_RAM_Table), segment->data.Id);
        print_to_buffer(&(context->buffer), "push [%lu]     ; Variable with name: \"%s\"\n", var_offset,
                                            find_in_name_table_by_code(&(table_array->Array[0]), segment->data.Id));

        return NO_LANG_ERRORS;
    }

    print_to_buffer(&(context->buffer), "push [rpx+%lu]     ; Variable with name: \"%s\"\n", var_offset,
                                        find_in_name_table_by_code(&(table_array->Array[0]), segment->data.Id));

    return NO_LANG_ERRORS;
}

static langErrorCode compile_function_call(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context)
{
    assert(segment);
    assert(table_array);
    assert(RAM_Table);
    assert(context);

    langErrorCode error = NO_LANG_ERRORS;

    if (segment->left)
    {
        if ((error = compile_parameters_in_call(segment->left, table_array, RAM_Table, context)))    
        {   
            return error;
        }
    }

    print_to_buffer(&(context->buffer), "; Saving memory stack pointer\n"
                            "push rpx       ; Save old stack pointr\n"
                            "push rpx\n"
                            "push %lu\n"
                            "add\n"
                            "pop rpx\n", RAM_Table->Pointer);

    print_to_buffer(&(context->buffer), "call Function_%lu\n", segment->right->data.Id);

    if (segment->parent->data.K_word != KEY_NEXT)
    {
        print_to_buffer(&(context->buffer), "push rax\n");
    }

    return error;
}

static langErrorCode compile_parameters_in_call(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context)
{
    assert(segment);
    assert(table_array);
    assert(RAM_Table);
    assert(context);

    langErrorCode error = NO_LANG_ERRORS; // BUG параметры должны быть уже определены и находиться в RAM_Table
    print_to_buffer(&(context->buffer), "; Compile parameter\n");
    if ((error = lang_compiler_recursive(segment->left, table_array, RAM_Table, context)))
    {
        return error;
    }

    if (segment->right)
    {
        if ((error = compile_parameters_in_call(segment->right, table_array, RAM_Table, context)))
        {
            return error;
        }
    }

    return error;
}

static langErrorCode compile_function_definition(const TreeSegment* segment, const LangNameTableArray* table_array, BackendContext* context)
{
    assert(segment);
    assert(table_array);
    assert(context);

    langErrorCode error = NO_LANG_ERRORS;

    memoryTable RAM_Table = {};                         // Инициализация локальной таблицы

    context->current_function = segment->data.Id;

    const char* function_name = find_in_name_table_by_code(&(table_array->Array[0]), segment->data.Id);

    print_to_buffer(&(context->buffer), "\n\n; Function declaration with name: %s\n", function_name);

    if (!strcmp("главная", function_name))
    {
        print_to_buffer(&(context->buffer), ":main\n");
    }
    else
    {
        print_to_buffer(&(context->buffer), ":Function_%lu\n", segment->data.Id);
    }

    const TreeSegment* parameter_segment = segment->right;

    if (parameter_segment->left)
    {
        print_to_buffer(&(context->buffer), "; Save old rpx value in rbx\n"
                                            "pop rbx\n"
                                            "; Accepting parameners\n");
        if ((error = compile_parameters_in_definition(parameter_segment->left, find_name_table(table_array, segment->right->data.Id), &RAM_Table, context)))
        {
            return error;
        }
        print_to_buffer(&(context->buffer), "; Repair old rpx value to stack\n"
                                            "push rbx\n");
    }

    if ((error = lang_compiler_recursive(parameter_segment->right, table_array, &RAM_Table, context)))
    {
        return error;
    }

    print_to_buffer(&(context->buffer), "pop rpx\n"
                                        "ret\n"); // Восстановление указателя на стековый кадр после обработки функции

    context->current_function = 0;    // Возвращиемся в глобальную область видимости

    return error;
}

static langErrorCode compile_parameters_in_definition(const TreeSegment* segment, const LangNameTable* name_table, memoryTable* RAM_Table, BackendContext* context)
{
    assert(segment);
    assert(name_table);
    assert(context);

    langErrorCode error = NO_LANG_ERRORS;

    if ((error = add_to_RAM_Table(RAM_Table, segment->left->left->data.Id)))
    {
        return error;
    }
    print_to_buffer(&(context->buffer), "; Parameter declaration with name: %s\n", find_in_name_table_by_code(name_table, segment->left->left->data.Id));
    print_to_buffer(&(context->buffer), "pop [rpx+%lu]\n", RAM_Table->Pointer - 1);

    if (segment->right)
    {
        if ((error = compile_parameters_in_definition(segment->right, name_table, RAM_Table, context)))
        {
            return error;
        }
    }

    return error;
}

static langErrorCode compile_var_declaration(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context)
{
    assert(segment);
    assert(table_array);
    assert(RAM_Table);
    assert(context);

    langErrorCode error = NO_LANG_ERRORS;

    // Проверка на глобальную область видимости

    if (context->current_function == 0)
    {
        return error;
    } 
    else if ((error = add_to_RAM_Table(RAM_Table, segment->data.Id)))
    {
        return error;
    }

    print_to_buffer(&(context->buffer), "; Defined variable with name: %s\n", find_in_name_table_by_code(&(table_array->Array[0]), segment->right->data.Id));

    if (segment->right->type == IDENTIFIER)
    {
        return error;
    }

    if ((error = lang_compiler_recursive(segment->right, table_array, RAM_Table, context)))
    {
        return error;
    }

    return error;
}

static langErrorCode compile_global_var_declaration(const TreeSegment* segment, const LangNameTableArray* table_array, BackendContext* context)
{
    assert(segment);
    assert(table_array);
    assert(context);

    langErrorCode error = NO_LANG_ERRORS;

    if ((error = add_to_RAM_Table(&(context->global_RAM_Table), segment->data.Id)))
    {
        return error;
    }

    print_to_buffer(&(context->buffer), "; Defined global variable with name: \"%s\"\n", find_in_name_table_by_code(&(table_array->Array[0]), segment->data.Id));

    if (segment->right->type == IDENTIFIER)
    {
        return error;
    }

    if ((error = lang_compiler_recursive(segment->right, table_array, &(context->global_RAM_Table), context)))
    {
        return error;
    }

    return error;
}

#define RECURSE_LEFT_BRANCH do{                                                                     \
    if (segment->left)                                                                              \
    {                                                                                               \
        if ((error = lang_compiler_recursive(segment->left, table_array, RAM_Table, context)))      \
        {                                                                                           \
            return error;                                                                           \
        }                                                                                           \
    }                                                                                               \
}while (0)

#define RECURSE_RIGHT_BRANCH do{                                                                    \
    if (segment->right)                                                                             \
    {                                                                                               \
        if ((error = lang_compiler_recursive(segment->right, table_array, RAM_Table, context)))     \
        {                                                                                           \
            return error;                                                                           \
        }                                                                                           \
    }                                                                                               \
}while(0)  

#define ADD_ZERO_BRANCH_COMMAND(_KEY_WORD_, _COMENT_TEXT_, _COMMAND_TEXT_)                          \
case _KEY_WORD_:                                                                                    \
    print_to_buffer(&(context->buffer), _COMENT_TEXT_);                                             \
                                                                                                    \
    _COMMAND_TEXT_                                                                                  \
    break;                                                                                          \

#define ADD_RIGHT_BRANCH_COMMAND(_KEY_WORD_, _COMENT_TEXT_, _COMMAND_TEXT_)                         \
case _KEY_WORD_:                                                                                    \
    print_to_buffer(&(context->buffer), _COMENT_TEXT_);                                             \
    RECURSE_RIGHT_BRANCH;                                                                           \
                                                                                                    \
    _COMMAND_TEXT_                                                                                  \
    break;                                                                                          \

#define ADD_DOUBLE_BRANCH_COMMAND(_KEY_WORD_, _COMENT_TEXT_, _COMMAND_TEXT_)                        \
    case _KEY_WORD_:                                                                                \
    print_to_buffer(&(context->buffer), _COMENT_TEXT_);                                             \
    RECURSE_LEFT_BRANCH;                                                                            \
    RECURSE_RIGHT_BRANCH;                                                                           \
                                                                                                    \
    _COMMAND_TEXT_                                                                                  \
    break;                                                                                          \

#define ADD_LEFT_BRANCH_COMMAND(_KEY_WORD_, _COMENT_TEXT_, _COMMAND_TEXT_)                          \
    case _KEY_WORD_:                                                                                \
    print_to_buffer(&(context->buffer), _COMENT_TEXT_);                                             \
    RECURSE_LEFT_BRANCH;                                                                            \
                                                                                                    \
    _COMMAND_TEXT_                                                                                  \
    break;                                                                                          \

static langErrorCode compile_keyword(const TreeSegment* segment, const LangNameTableArray* table_array, memoryTable* RAM_Table, BackendContext* context)
{
    assert(segment);
    assert(table_array);
    assert(context);

    langErrorCode error = NO_LANG_ERRORS;

    size_t temp_scope_counter = 0;
    size_t var_offset         = 0;

    switch ((size_t) segment->data.K_word)
    {

    ADD_ZERO_BRANCH_COMMAND(KEY_IN,             "; Print in()\n", print_to_buffer(&(context->buffer), "in\n"););
    ADD_ZERO_BRANCH_COMMAND(KEY_BREAK,          "",               print_to_buffer(&(context->buffer), "jmp Skip_scope_%lu     ; Break\n", context->last_while_scope););
    ADD_ZERO_BRANCH_COMMAND(KEY_CONTINUE,       "",               print_to_buffer(&(context->buffer), "jmp While_next_%lu     ; Continue\n", context->last_while_scope););

    ADD_LEFT_BRANCH_COMMAND(KEY_ASSIGMENT, "",
                            if (((var_offset = find_in_RAM_table(RAM_Table, segment->right->data.Id))) == (size_t) -1)
                            {
                                var_offset = find_in_RAM_table(&(context->global_RAM_Table), segment->right->data.Id);
                                print_to_buffer(&(context->buffer), "pop [%lu]\n", var_offset);
                                break;
                            }
                            print_to_buffer(&(context->buffer), "pop [rpx+%lu]\n", var_offset););

    ADD_RIGHT_BRANCH_COMMAND(KEY_OUT,           "; Print function\n",  print_to_buffer(&(context->buffer), "out\n"););
    ADD_RIGHT_BRANCH_COMMAND(KEY_SIN,           "; Print sin\n",       print_to_buffer(&(context->buffer), "sin\n"););
    ADD_RIGHT_BRANCH_COMMAND(KEY_COS,           "; Print cos\n",       print_to_buffer(&(context->buffer), "cos\n"););
    ADD_RIGHT_BRANCH_COMMAND(KEY_RETURN,        "; Return\n",          print_to_buffer(&(context->buffer), "pop rax\n"
                                                                                               "pop rpx\n"
                                                                                               "ret\n"););
    
    ADD_DOUBLE_BRANCH_COMMAND(KEY_NEXT,         "",                    ;);
    ADD_DOUBLE_BRANCH_COMMAND(KEY_PLUS,         "; Print summ node\n", print_to_buffer(&(context->buffer), "add\n"););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_MINUS,        "; Print summ node\n", print_to_buffer(&(context->buffer), "sub\n"););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_MUL,          "; Print summ node\n", print_to_buffer(&(context->buffer), "mul\n"););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_DIV,          "; Print summ node\n", print_to_buffer(&(context->buffer), "div\n"););

    ADD_DOUBLE_BRANCH_COMMAND(KEY_MORE,         "",                    print_to_buffer(&(context->buffer), "jbe Skip_scope_%lu\n", context->scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_MORE_EQUAL,   "",                    print_to_buffer(&(context->buffer), "jb Skip_scope_%lu\n",  context->scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_LESS,         "",                    print_to_buffer(&(context->buffer), "jae Skip_scope_%lu\n", context->scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_LESS_EQUAL,   "",                    print_to_buffer(&(context->buffer), "ja Skip_scope_%lu\n",  context->scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_EQUAL,        "",                    print_to_buffer(&(context->buffer), "jne Skip_scope_%lu\n", context->scope_counter););
    ADD_DOUBLE_BRANCH_COMMAND(KEY_NOT_EQUAL,    "",                    print_to_buffer(&(context->buffer), "je Skip_scope_%lu\n",  context->scope_counter););

    case KEY_IF:
        print_to_buffer(&(context->buffer), "; If %lu section\n", context->scope_counter);
        temp_scope_counter = context->scope_counter;                                           // save current scope counter
        RECURSE_LEFT_BRANCH;
        (context->scope_counter)++;
        RECURSE_RIGHT_BRANCH;
        print_to_buffer(&(context->buffer), ":Skip_scope_%lu        ; End of if %lu section\n\n", temp_scope_counter, temp_scope_counter);

        break;

    case KEY_WHILE:
        print_to_buffer(&(context->buffer), "; While %lu section\n"
                                            ":While_next_%lu\n", context->scope_counter, context->scope_counter);
        STACK_PUSH(&(context->while_stack), (elem_t) context->last_while_scope);               // save old last while scope number in stack
        context->last_while_scope = context->scope_counter;                                    // save last while scope number 
        temp_scope_counter = context->scope_counter;                                           // save current scope counter
        RECURSE_LEFT_BRANCH;
        (context->scope_counter)++;
        RECURSE_RIGHT_BRANCH;
        context->last_while_scope = (size_t) STACK_POP(&(context->while_stack));               // repair last while scope number from stack
        print_to_buffer(&(context->buffer), "jmp While_next_%lu\n"
                                            ":Skip_scope_%lu        ; End of if %lu section\n\n", temp_scope_counter, temp_scope_counter, temp_scope_counter);

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
    assert(RAM_Table);

    if (RAM_Table->Pointer >= RAM_SIZE)
    {
        return PROCESSOR_MEMORY_OVER;
    }

    RAM_Table->Var[RAM_Table->Pointer] = number;
    (RAM_Table->Pointer)++;

    return NO_LANG_ERRORS;
}

//#################################################################################################//
//------------------------------------> Shared functions <-----------------------------------------//
//#################################################################################################//

static langErrorCode BackendContext_ctor(BackendContext* context, FILE* asm_file)
{
    assert(context);

    STACK_CTOR(&(context->while_stack), START_WHILE_STACK_SIZE);

    context->buffer.filePointer = asm_file;
    context->buffer.AUTO_FLUSH  = true;

    context->last_while_scope = 0;
    context->scope_counter    = 0;
    context->current_function = 0;

    return NO_LANG_ERRORS;
}