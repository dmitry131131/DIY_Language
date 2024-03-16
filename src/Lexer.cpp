#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>

#include "Tree.h"
#include "LangErrors.h"
#include "FrontEnd.h"
#include "Lexer.h"

static langErrorCode read_lang_punct_command(outputBuffer* buffer, LangToken* token);
static langErrorCode read_lang_text_command(outputBuffer* buffer, LangToken* token);

static langErrorCode add_line_to_array(LangTokenArray* token_array, size_t position);

langErrorCode lang_lexer(LangTokenArray* token_array, outputBuffer* buffer)
{
    assert(token_array);
    assert(buffer);
    langErrorCode error = NO_LANG_ERRORS;

    size_t count = 0;
    while (buffer->customBuffer[buffer->bufferPointer] != '\0')
    {
        if (isdigit(buffer->customBuffer[buffer->bufferPointer]))
        {
            ((token_array->Array)[count]).type     = NUM;
            ((token_array->Array)[count]).position = buffer->bufferPointer;

            int digit_len = 0;
            sscanf(buffer->customBuffer +buffer->bufferPointer, "%lf%n", &(((token_array->Array)[count]).data.num), &digit_len);
            buffer->bufferPointer += (size_t) digit_len;

            count++;
        }
        else if (ispunct(buffer->customBuffer[buffer->bufferPointer]))
        {
            if ((error = read_lang_punct_command(buffer, &(token_array->Array)[count])))
            {
                return error;
            }

            count++;
        }
        else if (!isspace(buffer->customBuffer[buffer->bufferPointer]))
        {
            if ((error = read_lang_text_command(buffer, &(token_array->Array)[count])))
            {
                return error;
            }

            count++;
        }
        else if (buffer->customBuffer[buffer->bufferPointer] == '\n')
        {
            (buffer->bufferPointer)++;
            if ((error = add_line_to_array(token_array, buffer->bufferPointer)))
            {
                return error;
            }
        }
        else
        {
            (buffer->bufferPointer)++;
        }
    }

    token_array->size = count;
    return NO_LANG_ERRORS;
}

static langErrorCode read_lang_punct_command(outputBuffer* buffer, LangToken* token)
{
    assert(buffer);
    assert(token);

    #define RECOGNIZE_SINGLE_COMMAND(CMD, KW)                                           \
        case CMD:                                                                       \
            token->type = KEY_WORD;                                                     \
            token->data.K_word = KW;                                                    \
            break;

    #define RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND(FIRST, SECOND, IF_DOUBLE, IF_SINGLE)     \
        case FIRST:                                                                     \
            token->type = KEY_WORD;                                                     \
            if (buffer->customBuffer[buffer->bufferPointer + 1] == SECOND)              \
            {                                                                           \
                token->data.K_word = IF_DOUBLE;                                         \
                (buffer->bufferPointer)++;                                              \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                token->data.K_word = IF_SINGLE;                                         \
            }                                                                           \
            break;             

    #define RECOGNIZE_DOUBLE_COMMAND(CMD, KW)                                           \
        case CMD:                                                                       \
            if (buffer->customBuffer[buffer->bufferPointer + 1] == CMD)                 \
            {                                                                           \
                token->type = KEY_WORD;                                                 \
                token->data.K_word = KW;                                                \
                (buffer->bufferPointer)++;                                              \
            }                                                                           \
            break;                               

    token->position = buffer->bufferPointer;

    switch (buffer->customBuffer[buffer->bufferPointer])
    {
    RECOGNIZE_SINGLE_COMMAND('(', KEY_OBR)
    RECOGNIZE_SINGLE_COMMAND(')', KEY_CBR)
    RECOGNIZE_SINGLE_COMMAND('{', KEY_O_CURBR)
    RECOGNIZE_SINGLE_COMMAND('}', KEY_C_CURBR)
    
    RECOGNIZE_SINGLE_COMMAND('+', KEY_PLUS)
    RECOGNIZE_SINGLE_COMMAND('-', KEY_MINUS)
    RECOGNIZE_SINGLE_COMMAND('*', KEY_MUL)
    RECOGNIZE_SINGLE_COMMAND('/', KEY_DIV)
    RECOGNIZE_SINGLE_COMMAND(';', KEY_NEXT)
    RECOGNIZE_SINGLE_COMMAND(',', KEY_ENUM)

    //RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND('=', '=', KEY_EQUAL, KEY_ASSIGMENT)
    RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND('<', '=', KEY_LESS_EQUAL, KEY_LESS)
    RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND('>', '=', KEY_MORE_EQUAL, KEY_MORE)
    RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND('!', '=', KEY_NOT_EQUAL, KEY_NOT)

    RECOGNIZE_DOUBLE_COMMAND('|', KEY_OR)
    RECOGNIZE_DOUBLE_COMMAND('&', KEY_AND)
    RECOGNIZE_DOUBLE_COMMAND('=', KEY_EQUAL)

    default:
        return WRONG_LANG_SYNTAX;
        break;
    }

    (buffer->bufferPointer)++;
    return NO_LANG_ERRORS;

    #undef RECOGNIZE_DOUBLE_COMMAND
    #undef RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND
    #undef RECOGNIZE_SINGLE_COMMAND
}

static langErrorCode read_lang_text_command(outputBuffer* buffer, LangToken* token)
{
    assert(buffer);
    assert(token);

    #define NEW_KEY_WORD(text, KW)          \
    else if (!strcmp(cmd, text))            \
    {                                       \
        token->type        = KEY_WORD;      \
        token->data.K_word = KW;            \
    }                                       

    char cmd[MAX_LANG_COMMAND_LEN] = {};
    size_t len = 0;

    token->position = buffer->bufferPointer;

    while(!isspace(buffer->customBuffer[buffer->bufferPointer])
    &&    !ispunct(buffer->customBuffer[buffer->bufferPointer])
    &&    !isdigit(buffer->customBuffer[buffer->bufferPointer]))
    {
        cmd[len] = buffer->customBuffer[buffer->bufferPointer];
        len++;
        (buffer->bufferPointer)++;
    }
    cmd[len] = '\0';

    if (!strcmp(cmd, "синус"))
    {
        token->type        = KEY_WORD;
        token->data.K_word = KEY_SIN;
    }
    NEW_KEY_WORD("косинус",             KEY_COS)
    NEW_KEY_WORD("компаратор",          KEY_IF)
    NEW_KEY_WORD("контур",              KEY_WHILE)
    NEW_KEY_WORD("заземлить",           KEY_FLOOR)
    NEW_KEY_WORD("разобрать",           KEY_DIFF)
    NEW_KEY_WORD("бит",                 KEY_NUMBER)
    NEW_KEY_WORD("схема",               KEY_DEF)
    NEW_KEY_WORD("впаять",              KEY_IN)
    NEW_KEY_WORD("вывести",             KEY_OUT)
    NEW_KEY_WORD("приклеить",           KEY_ASSIGMENT)
    NEW_KEY_WORD("вернуть",             KEY_RETURN)
    NEW_KEY_WORD("отрезать",            KEY_BREAK)
    NEW_KEY_WORD("повтор",              KEY_CONTINUE)
    else
    {
        token->type      = ID;
        token->data.text = strdup(cmd);
    }

    return NO_LANG_ERRORS;

    #undef NEW_KEY_WORD
}

static langErrorCode line_beginings_realloc(LangTokenArray* token_array)
{
    assert(token_array);

    token_array->line_beginings = (size_t*) realloc(token_array->line_beginings, token_array->line_array_size * 2);
    token_array->line_array_size *= 2;
    if (!token_array->line_beginings)
    {
        return LINE_ARRAY_ALLOC_ERROR;
    }

    return NO_LANG_ERRORS;
}

static langErrorCode add_line_to_array(LangTokenArray* token_array, size_t position)
{
    assert(token_array);
    langErrorCode error = NO_LANG_ERRORS;

    (token_array->line_count)++;

    if (token_array->line_count >= token_array->line_array_size)
    {
        if ((error = line_beginings_realloc(token_array)))
        {
            return error;
        }
    }

    token_array->line_beginings[token_array->line_count] = position;

    return error;
}