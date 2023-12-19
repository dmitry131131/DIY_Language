#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "Tree.h"
#include "DataBuffer.h"
#include "LangErrors.h"
#include "FrontEnd.h"

static langErrorCode lang_lexer(LangTokenArray* token_array, outputBuffer* buffer);

static langErrorCode read_lang_punct_command(outputBuffer* buffer, LangToken* token);

static langErrorCode token_array_dtor(LangTokenArray* token_array);

static langErrorCode read_lang_text_command(outputBuffer* buffer, LangToken* token);

langErrorCode lang_parser(const char* filename, TreeData* tree)
{
    assert(filename);
    assert(tree);

    #define RETURN(code) do{                \
        token_array_dtor(&token_array);     \
        if (buffer_dtor(&buffer))           \
        {                                   \
            return DTOR_LANG_BUFFER_ERROR;  \
        }                                   \
        return code;                        \
    }while(0)

    langErrorCode error = NO_LANG_ERRORS;

    FILE* file = fopen(filename, "r");
    if (!file)
    {
        return LANG_FILE_OPEN_ERROR;
    }

    outputBuffer buffer = {};

    if (read_file_in_buffer(&buffer, file))
    {
        return READ_FROM_LANG_FILE_ERROR;
    }
    fclose(file);

    LangTokenArray token_array = {};

    token_array.Array = (LangToken*) calloc(buffer.customSize + 2, sizeof(LangToken));
    
    if ((error = lang_lexer(&token_array, &buffer)))
    {
        RETURN(error);
    }

    printf("%lf\n", token_array.Array[2].data.num);
    /*
    if ((error = getG(tree, &token_array)))
    {
        printf("In position: %lu\n", token_array.Array[token_array.Pointer].position);
        RETURN(error);
    }
    */
    RETURN(NO_LANG_ERRORS);

    #undef RETURN
}

static langErrorCode token_array_dtor(LangTokenArray* token_array)
{
    assert(token_array);

    for (size_t i = 0; i < token_array->size; i++)
    {
        if (token_array->Array[i].type == ID)
        {
            free(token_array->Array[i].data.text);
        }
    }
    free(token_array->Array);

    return NO_LANG_ERRORS;
}   

static langErrorCode lang_lexer(LangTokenArray* token_array, outputBuffer* buffer)
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
        else if (isalpha(buffer->customBuffer[buffer->bufferPointer]))
        {
            if ((error = read_lang_text_command(buffer, &(token_array->Array)[count])))
            {
                return error;
            }

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

    #define RECOGNIZE_SINGLE_COMMAND(CMD, KW)                   \
        case CMD:                                               \
            token->type = KEY_WORD;                             \
            token->data.K_word = KW;                            \
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
    case '(':
        token->type = OP;
        token->data.op = OBR;
        break;
    case ')':
        token->type = OP;
        token->data.op = CBR;
        break;
    RECOGNIZE_SINGLE_COMMAND('+', KEY_PLUS)
    RECOGNIZE_SINGLE_COMMAND('-', KEY_MINUS)
    RECOGNIZE_SINGLE_COMMAND('*', KEY_MUL)
    RECOGNIZE_SINGLE_COMMAND('/', KEY_DIV)
    RECOGNIZE_SINGLE_COMMAND(';', KEY_NEXT)
    RECOGNIZE_SINGLE_COMMAND(',', KEY_ENUM)

    RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND('=', '=', KEY_EQUAL, KEY_ASSIGMENT)
    RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND('<', '=', KEY_LESS_EQUAL, KEY_LESS)
    RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND('>', '=', KEY_MORE_EQUAL, KEY_MORE)
    RECOGNIZE_SINGLE_OR_DOUBLE_COMMAND('!', '=', KEY_NOT_EQUAL, KEY_NOT)

    RECOGNIZE_DOUBLE_COMMAND('|', KEY_OR)
    RECOGNIZE_DOUBLE_COMMAND('&', KEY_AND)

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

    while(isalpha(buffer->customBuffer[buffer->bufferPointer]))
    {
        cmd[len] = buffer->customBuffer[buffer->bufferPointer];
        len++;
        (buffer->bufferPointer)++;
    }
    cmd[len] = '\0';

    if (!strcmp(cmd, "sin"))
    {
        token->type        = KEY_WORD;
        token->data.K_word = KEY_SIN;
    }
    NEW_KEY_WORD("cos", KEY_COS)
    NEW_KEY_WORD("if", KEY_IF)
    NEW_KEY_WORD("while", KEY_WHILE)
    NEW_KEY_WORD("floor", KEY_FLOOR)
    NEW_KEY_WORD("diff", KEY_DIFF)
    NEW_KEY_WORD("number", KEY_NUMBER)
    NEW_KEY_WORD("==", KEY_EQUAL)
    NEW_KEY_WORD("<=", KEY_LESS_EQUAL)
    NEW_KEY_WORD(">=", KEY_MORE_EQUAL)
    NEW_KEY_WORD("!=", KEY_NOT_EQUAL)
    NEW_KEY_WORD("&&", KEY_AND)
    NEW_KEY_WORD("||", KEY_OR)

    else
    {
        token->type      = ID;
        token->data.text = strdup(cmd);
    }

    return NO_LANG_ERRORS;

    #undef NEW_KEY_WORD
}