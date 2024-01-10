#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>

#include "Tree.h"
#include "DataBuffer.h"
#include "LangErrors.h"
#include "FrontEnd.h"
#include "Output.h"

static langErrorCode lang_lexer(LangTokenArray* token_array, outputBuffer* buffer);
static langErrorCode read_lang_punct_command(outputBuffer* buffer, LangToken* token);
static langErrorCode token_array_dtor(LangTokenArray* token_array);
static langErrorCode read_lang_text_command(outputBuffer* buffer, LangToken* token);
static TreeSegment*  getDecl(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getFuncDeclaration(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getDeclaration(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  MainTrans(LangTokenArray* token_array, langErrorCode* error);
static langErrorCode getG(TreeData* tree, LangTokenArray* token_array);
static TreeSegment*  CreateNode(SegmemtType type, SegmentData data, TreeSegment* left, TreeSegment* right);
static TreeSegment*  getE(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getAE(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getPriority1(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getPriority2(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getPriority3(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getPriority4(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getIf(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getWhile(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getOperatorList(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getOper(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getFuncCall(LangTokenArray* token_array, langErrorCode* error);
static TreeSegment*  getOut(LangTokenArray* token_array, langErrorCode* error);

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

    if ((error = getG(tree, &token_array)))
    {
        printf("In position: %lu\n", token_array.Array[token_array.Pointer].position);
        RETURN(error);
    }

    write_lang_tree_to_file("out.txt", tree);

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
    else
    {
        token->type      = ID;
        token->data.text = strdup(cmd);
    }

    return NO_LANG_ERRORS;

    #undef NEW_KEY_WORD
}

static TreeSegment* CreateNode(SegmemtType type, SegmentData data, TreeSegment* left, TreeSegment* right)
{
    TreeSegment* seg = new_segment(type, sizeof(int));

    seg->data  = data;
    seg->left  = left;
    seg->right = right;

    return seg;
}

static langErrorCode getG(TreeData* tree, LangTokenArray* token_array)
{
    assert(tree);
    assert(token_array);

    langErrorCode error = NO_LANG_ERRORS;

    tree->root = MainTrans(token_array, &error);

    return error;
}

static TreeSegment* MainTrans(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = getDeclaration(token_array, error);
    if (*error) return val;
    
    if ((token_array->Pointer + 1) < token_array->size)
    {
        val->right = MainTrans(token_array, error);
        if (*error) return val;
    }

    return val;
}

static TreeSegment* getDeclaration(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = nullptr;

    if ((token_array->Array)[token_array->Pointer].type != KEY_WORD)
    {
        *error = WRONG_LANG_SYNTAX;
        return val;
    }

    if ((token_array->Array)[token_array->Pointer].data.K_word == KEY_DEF)
    {
        val = getFuncDeclaration(token_array, error);
    }
    else if ((token_array->Array)[token_array->Pointer].data.K_word == KEY_NUMBER)
    {
        val = getDecl(token_array, error);
    }
    else
    {
        *error = WRONG_LANG_SYNTAX;
        return val;
    }

    return val;
}

#define CHECK_BRACKET(BR_TYPE, DEL_CODE) do{                                \
    if ((token_array->Array)[token_array->Pointer].type != KEY_WORD         \
    ||  (token_array->Array)[token_array->Pointer].data.K_word != BR_TYPE)  \
    {                                                                       \
        *error = WRONG_LANG_SYNTAX;                                         \
        DEL_CODE                                                            \
        return nullptr;                                                     \
    }                                                                       \
}while(0)

#define CHECK_KEY_WORD(KW, DEL_CODE) do{                                    \
    if ((token_array->Array)[token_array->Pointer].type != KEY_WORD         \
    ||  (token_array->Array)[token_array->Pointer].data.K_word != KW)       \
    {                                                                       \
        *error = WRONG_LANG_SYNTAX;                                         \
        DEL_CODE                                                            \
        return nullptr;                                                     \
    }                                                                       \
}while(0)

static TreeSegment* getFuncDeclaration(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    CHECK_KEY_WORD(KEY_DEF, ;);

    (token_array->Pointer)++;
    
    CHECK_KEY_WORD(KEY_NUMBER, ;);
    
    SegmentData data = {.K_word = (token_array->Array)[token_array->Pointer].data.K_word};
    TreeSegment* val2 = CreateNode(KEYWORD, data, nullptr, nullptr);
    (token_array->Pointer)++;

    if ((token_array->Array)[token_array->Pointer].type != ID)
    {
        *error = WRONG_LANG_SYNTAX;
        del_segment(val2);
        return nullptr;
    }
    data.stringPtr = strdup((token_array->Array)[token_array->Pointer].data.text);
    TreeSegment* val = CreateNode(FUNCTION_DEFINITION, data, val2, nullptr);
    (token_array->Pointer)++;

    CHECK_BRACKET(KEY_OBR, del_segment(val););
    (token_array->Pointer)++;

    // Function arguments

    CHECK_BRACKET(KEY_CBR, del_segment(val););
    (token_array->Pointer)++;

    val2 = getOper(token_array, error);
    if (*error)
    {
        del_segment(val);
        return val2;
    }
    
    data.stringPtr = NULL;
    val2 = CreateNode(PARAMETERS, data, nullptr, val2);

    val->right = val2;

    data.K_word = KEY_NEXT;
    val = CreateNode(KEYWORD, data, val, nullptr); 

    return val;
}

static TreeSegment* getDecl(LangTokenArray* token_array, langErrorCode* error)
{   
    assert(token_array);
    assert(error);

    CHECK_KEY_WORD(KEY_NUMBER, ;);

    SegmentData data = {.K_word = (token_array->Array)[token_array->Pointer].data.K_word};
    TreeSegment* val = CreateNode(KEYWORD, data, nullptr, nullptr);
    (token_array->Pointer)++;

    if ((token_array->Array)[token_array->Pointer].type     != ID
    ||  (token_array->Array)[token_array->Pointer + 1].type != KEY_WORD)
    {
        *error = WRONG_LANG_SYNTAX;
        del_segment(val);
        return nullptr;
    }

    TreeSegment* val2 = nullptr;
    if ((token_array->Array)[token_array->Pointer + 1].data.K_word == KEY_ASSIGMENT)
    {
        data.stringPtr = strdup((token_array->Array)[token_array->Pointer].data.text);
        val2 = getAE(token_array, error);
        (token_array->Pointer)++;
    }
    else if ((token_array->Array)[token_array->Pointer + 1].data.K_word == KEY_NEXT)
    {
        data.stringPtr = strdup((token_array->Array)[token_array->Pointer].data.text);
        val2 = CreateNode(IDENTIFIER, data, nullptr, nullptr);
        data.stringPtr = strdup((token_array->Array)[token_array->Pointer].data.text);
        (token_array->Pointer) += 2;
        printf("%lu", token_array->Pointer);
    }
    else
    {
        *error = WRONG_LANG_SYNTAX;
        del_segment(val);
        return nullptr;
    }

    val = CreateNode(VAR_DECLARATION, data, val, val2);

    data.K_word = KEY_NEXT;
    val = CreateNode(KEYWORD, data, val, nullptr);

    return val;
}

static TreeSegment* getAE(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    if ((token_array->Array)[token_array->Pointer + 1].data.K_word != KEY_ASSIGMENT
    ||  (token_array->Array)[token_array->Pointer].type            != ID)
    {
        *error = WRONG_LANG_SYNTAX;
        return nullptr;
    }

    SegmentData data = {.stringPtr = strdup((token_array->Array)[token_array->Pointer].data.text)};
    TreeSegment* val = CreateNode(IDENTIFIER, data, nullptr, nullptr);
    
    (token_array->Pointer) += 2;
    
    TreeSegment* val2 = getE(token_array, error);
    if (*error)
    {
        del_segment(val);
        return val2;
    }

    data.K_word = KEY_ASSIGMENT;
    val = CreateNode(KEYWORD, data, val2, val);

    return val;
}

#define CHECK_ERROR_AND_CREATE_NODE do{             \
    if (*error)                                     \
    {                                               \
        del_segment(val);                           \
        return val2;                                \
    }                                               \
                                                    \
    SegmentData data = {.K_word = temp_key_word};   \
    val = CreateNode(KEYWORD, data, val, val2);     \
}while(0)

static TreeSegment* getE(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = getPriority4(token_array, error);
    if (*error) return val;

    while ((token_array->Array)[token_array->Pointer].type == KEY_WORD 
    &&    ((token_array->Array)[token_array->Pointer].data.K_word == KEY_AND 
    ||     (token_array->Array)[token_array->Pointer].data.K_word == KEY_OR))
    {
        KeyWords temp_key_word = (token_array->Array)[token_array->Pointer].data.K_word;
        (token_array->Pointer)++;

        TreeSegment* val2 = getPriority4(token_array, error);
        CHECK_ERROR_AND_CREATE_NODE;
    }

    return val;
}

static TreeSegment* getPriority4(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = getPriority3(token_array, error);
    if (*error) return val;

    if ((token_array->Array)[token_array->Pointer].type == KEY_WORD 
    && ((token_array->Array)[token_array->Pointer].data.K_word == KEY_EQUAL 
    ||  (token_array->Array)[token_array->Pointer].data.K_word == KEY_MORE
    ||  (token_array->Array)[token_array->Pointer].data.K_word == KEY_MORE_EQUAL
    ||  (token_array->Array)[token_array->Pointer].data.K_word == KEY_LESS
    ||  (token_array->Array)[token_array->Pointer].data.K_word == KEY_LESS_EQUAL
    ||  (token_array->Array)[token_array->Pointer].data.K_word == KEY_NOT_EQUAL))
    {
        KeyWords temp_key_word = (token_array->Array)[token_array->Pointer].data.K_word;
        (token_array->Pointer)++;

        TreeSegment* val2 = getPriority3(token_array, error);
        CHECK_ERROR_AND_CREATE_NODE;
    }

    return val;
}

static TreeSegment* getPriority3(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = getPriority2(token_array, error);
    if (*error) return val;

    while ((token_array->Array)[token_array->Pointer].type == KEY_WORD 
    &&    ((token_array->Array)[token_array->Pointer].data.K_word == KEY_PLUS 
    ||     (token_array->Array)[token_array->Pointer].data.K_word == KEY_MINUS))
    {
        KeyWords temp_key_word = (token_array->Array)[token_array->Pointer].data.K_word;
        (token_array->Pointer)++;

        TreeSegment* val2 = getPriority2(token_array, error);
        CHECK_ERROR_AND_CREATE_NODE;
    }

    return val;
}

static TreeSegment* getPriority2(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = getPriority1(token_array, error);
    if (*error) return val;

    while ((token_array->Array)[token_array->Pointer].type == KEY_WORD 
    &&    ((token_array->Array)[token_array->Pointer].data.K_word == KEY_MUL 
    ||     (token_array->Array)[token_array->Pointer].data.K_word == KEY_DIV))
    {
        KeyWords temp_key_word = (token_array->Array)[token_array->Pointer].data.K_word;
        (token_array->Pointer)++;

        TreeSegment* val2 = getPriority1(token_array, error);
        CHECK_ERROR_AND_CREATE_NODE;
    }

    return val;
}

#undef CHECK_ERROR_AND_CREATE_NODE

static TreeSegment* getPriority1(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = nullptr;

    if ((token_array->Array)[token_array->Pointer].type == KEY_WORD)
    {
        SegmentData data = {.K_word = (token_array->Array)[token_array->Pointer].data.K_word};
        (token_array->Pointer)++;

        if (data.K_word != KEY_SIN && data.K_word != KEY_COS && data.K_word != KEY_FLOOR && data.K_word != KEY_IN && data.K_word != KEY_OBR)
        {
            *error = WRONG_LANG_SYNTAX;
            return nullptr;
        }

        if (data.K_word == KEY_OBR)
        {
            val = getE(token_array, error);
            if (*error) return val;
            CHECK_BRACKET(KEY_CBR, del_segment(val););
        }
        else
        {
            CHECK_BRACKET(KEY_OBR, ;);
            (token_array->Pointer)++;
            if (data.K_word != KEY_IN)
            {
                val = getE(token_array, error);
                if (*error) return val;
            }
            CHECK_BRACKET(KEY_CBR, del_segment(val););
            val = CreateNode(KEYWORD, data, nullptr, val);
        }
    }
    //TODO сделать правильное распознавание идентификатора через таблицу имён
    else if ((token_array->Array)[token_array->Pointer].type == ID)
    {
        if ((token_array->Array)[token_array->Pointer + 1].type        == KEY_WORD
        &&  (token_array->Array)[token_array->Pointer + 1].data.K_word == KEY_OBR)
        {
            val = getFuncCall(token_array, error);
            if (*error) return val;
        }

        SegmentData data = {.stringPtr = strdup((token_array->Array)[token_array->Pointer].data.text)};
        val = CreateNode(IDENTIFIER, data, nullptr, nullptr);
    }
    else if ((token_array->Array)[token_array->Pointer].type == NUM)
    {
        SegmentData data = {.D_number = (token_array->Array)[token_array->Pointer].data.num};
        val = CreateNode(DOUBLE_SEGMENT_DATA, data, nullptr, nullptr);
    }
    else
    {
        *error = NO_LANG_ERRORS;
        return val;
    }

    (token_array->Pointer)++;

    return val;
}

static TreeSegment* getOut(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = nullptr;

    CHECK_KEY_WORD(KEY_OUT, ;);
    (token_array->Pointer)++;

    CHECK_BRACKET(KEY_OBR, ;);
    (token_array->Pointer)++;

    val = getE(token_array, error);
    if (*error) return val;

    CHECK_BRACKET(KEY_CBR, del_segment(val););
    (token_array->Pointer)++;

    CHECK_KEY_WORD(KEY_NEXT, del_segment(val););
    (token_array->Pointer)++;

    SegmentData data = {.K_word = KEY_OUT};
    val = CreateNode(KEYWORD, data, nullptr, val);

    data.K_word = KEY_NEXT;
    val = CreateNode(KEYWORD, data, val ,nullptr);

    return val;
}

static TreeSegment* getFuncCall(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val  = nullptr;
    TreeSegment* val2 = nullptr;

    if ((token_array->Array)[token_array->Pointer].type != ID)
    {
        *error = WRONG_LANG_SYNTAX;
        return val;
    }
    SegmentData data = {};
    val = CreateNode(CALL, data, nullptr, nullptr);

    data.stringPtr = strdup((token_array->Array)[token_array->Pointer].data.text);
    val2 = CreateNode(IDENTIFIER, data, nullptr, nullptr);
    (token_array->Pointer)++;

    val->right = val2;

    data.K_word = KEY_ENUM;
    val2 = CreateNode(KEYWORD, data, nullptr, nullptr);

    val->left = val2;

    CHECK_BRACKET(KEY_OBR, del_segment(val););
    (token_array->Pointer)++;
    CHECK_BRACKET(KEY_CBR, del_segment(val););
    (token_array->Pointer)++;

    return val;
}

static TreeSegment* getIf(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    CHECK_KEY_WORD(KEY_IF, ;);
    (token_array->Pointer)++;

    CHECK_BRACKET(KEY_OBR, ;);
    (token_array->Pointer)++;

    TreeSegment* val = getE(token_array, error);
    if (*error) return val;

    CHECK_BRACKET(KEY_CBR, del_segment(val););
    (token_array->Pointer)++;

    TreeSegment* val2 = getOper(token_array, error);
    if (*error)
    {
        del_segment(val);
        return val2;
    }

    SegmentData data = {.K_word = KEY_IF};
    val = CreateNode(KEYWORD, data, val, val2);

    data.K_word = KEY_NEXT;
    val = CreateNode(KEYWORD, data, val, nullptr);

    return val;
}

static TreeSegment* getWhile(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    CHECK_KEY_WORD(KEY_WHILE, ;);
    (token_array->Pointer)++;

    CHECK_BRACKET(KEY_OBR, ;);
    (token_array->Pointer)++;

    TreeSegment* val = getE(token_array, error);
    if (*error) return val;

    CHECK_BRACKET(KEY_CBR, del_segment(val););
    (token_array->Pointer)++;

    TreeSegment* val2 = getOper(token_array, error);
    if (*error)
    {
        del_segment(val);
        return val2;
    }

    SegmentData data = {.K_word = KEY_WHILE};
    val = CreateNode(KEYWORD, data, val, val2);

    data.K_word = KEY_NEXT;
    val = CreateNode(KEYWORD, data, val, nullptr);

    return val;
}

static TreeSegment* getOper(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = nullptr;

    if ((token_array->Array)[token_array->Pointer].type == ID)
    {
        if ((token_array->Array)[token_array->Pointer + 1].type        == KEY_WORD
        &&  (token_array->Array)[token_array->Pointer + 1].data.K_word == KEY_OBR)
        {
            val = getFuncCall(token_array, error);
            if (*error) return val;
        }
        else
        {
            val = getAE(token_array, error);
            if (*error) return val;
        }
        
        CHECK_KEY_WORD(KEY_NEXT, ;);
        (token_array->Pointer)++;

        SegmentData data = {.K_word = KEY_NEXT};
        val = CreateNode(KEYWORD, data, val, nullptr);
    }
    else if ((token_array->Array)[token_array->Pointer].type == KEY_WORD)
    {
        SegmentData data = {.K_word = KEY_NEXT};
        switch ((int) (token_array->Array)[token_array->Pointer].data.K_word)
        {
        case KEY_IF:
            val = getIf(token_array, error);
            if (*error) return val;
            break;

        case KEY_WHILE:
            val = getWhile(token_array, error);
            if (*error) return val;
            break;

        case KEY_NUMBER:
            val = getDecl(token_array, error);
            if (*error) return val;
            break;

        case KEY_OUT:
            val = getOut(token_array, error);
            if (*error) return val;
            break;

        case KEY_O_CURBR:
            (token_array->Pointer)++;

            val = getOperatorList(token_array, error);
            if (*error) return val;
            CHECK_BRACKET(KEY_C_CURBR, del_segment(val););
            (token_array->Pointer)++;
            break;

        case KEY_C_CURBR:
            val = CreateNode(KEYWORD, data, nullptr, nullptr);
            break;
        
        default:
            *error = WRONG_LANG_SYNTAX;
            return val;
        }
    }
    else
    {
        (*error) = WRONG_LANG_SYNTAX;
        return val;
    }

    return val;
}

static TreeSegment* getOperatorList(LangTokenArray* token_array, langErrorCode* error)
{
    assert(token_array);
    assert(error);

    TreeSegment* val = getOper(token_array, error);
    if (*error) return val;
    
    TreeSegment* val2 = nullptr;
    
    if ((token_array->Array)[token_array->Pointer].type        == ID
    || ((token_array->Array)[token_array->Pointer].type        == KEY_WORD
    &&  (token_array->Array)[token_array->Pointer].data.K_word != KEY_C_CURBR))
    {
        val2 = getOperatorList(token_array, error);
        if (*error)
        {
            del_segment(val);
            return val2;
        }
    }
    
    val->right = val2;
    return val;
}

#undef CHECK_BRACKET