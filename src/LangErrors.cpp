#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "Color_output.h"
#include "LangErrors.h"

void print_lang_error(FILE* stream, langErrorCode error, size_t line, const char* text)
{
    assert(stream);
    #define PRINT_ERROR(error, errorCod, message) do{                   \
        if ((error) & (errorCod))                                       \
        {                                                               \
            color_fprintf(stream, COLOR_RED, STYLE_BOLD, "Error: ");    \
            fprintf(stream, message);                                   \
        }                                                               \
    }while(0)

    #define PRINT_SYNTAX_ERROR(error, CODE, message) do{                                            \
        if ((error) & CODE)                                                                         \
        {                                                                                           \
            color_fprintf(stream, COLOR_RED, STYLE_UNDERLINED, "Syntax error on line:");            \
            fprintf(stream, " ");                                                                   \
            color_fprintf(stream, COLOR_DEFAULT, STYLE_INVERT_C, "%lu", line);                      \
            fprintf(stream, " ");                                                                   \
            fprintf(stream, message);                                                               \
        }                                                                                           \
    }while(0)

    #define PRINT_SYNTAX_ERROR_ARG(error, CODE, message, text) do {                                 \
        if ((error) & CODE)                                                                         \
        {                                                                                           \
            color_fprintf(stream, COLOR_RED, STYLE_UNDERLINED, "Syntax error on line:");            \
            fprintf(stream, " ");                                                                   \
            color_fprintf(stream, COLOR_DEFAULT, STYLE_INVERT_C, "%lu", line);                      \
            fprintf(stream, " ");                                                                   \
            fprintf(stream, message, text);                                                         \
        }                                                                                           \
    }while(0)
 
    PRINT_ERROR(error, LANG_FILE_OPEN_ERROR,                    "Error in open file process!\n");
    PRINT_ERROR(error, READ_FROM_LANG_FILE_ERROR,               "Error in reading file process!\n");
    PRINT_ERROR(error, DTOR_LANG_BUFFER_ERROR,                  "Error in buffer destructor!\n");
    PRINT_ERROR(error, WRONG_LANG_SYNTAX,                       "Wrong lang syntax!\n");
    PRINT_ERROR(error, NAME_TABLE_ALLOC_MEMORY_ERROR,           "Error in calloc, tryed to alloc memory for name table\n");
    PRINT_ERROR(error, BAD_NAME_TABLE,                          "Bad name table!\n");
    PRINT_ERROR(error, UNDEFINED_NAME,                          "Undefined name!\n");
 
    PRINT_SYNTAX_ERROR(error, ID_EXPECTED,                      "ID was expected!\n");
    PRINT_SYNTAX_ERROR(error, KEYWORD_EXPECTED,                 "Key word was expected!\n");
 
    PRINT_SYNTAX_ERROR_ARG(error, WRONG_KEY_WORD,               "Wrong key word. Expected \"%s\"\n", text);
    PRINT_SYNTAX_ERROR_ARG(error, FUNCTION_REDECLARATION_ERROR, "Function with name \"%s\" already exist!\n", text);
    PRINT_SYNTAX_ERROR_ARG(error, VARIABLE_REDECLARATION_ERROR, "Variable with name \"%s\" already exist!\n", text);

    #undef PRINT_ERROR
    #undef PRINT_SYNTAX_ERROR
}
