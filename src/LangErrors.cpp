#include <stdio.h>
#include <assert.h>

#include "Color_output.h"
#include "LangErrors.h"

void print_lang_error(FILE* stream, langErrorCode error)
{
    assert(stream);
    #define PRINT_ERROR(error, errorCod, message) do{                   \
        if ((error) & (errorCod))                                       \
        {                                                               \
            color_fprintf(stream, COLOR_RED, STYLE_BOLD, "Error: ");    \
            fprintf(stream, message);                                   \
        }                                                               \
    }while(0)

    PRINT_ERROR(error, LANG_FILE_OPEN_ERROR,         "Error in open file process!\n");
    PRINT_ERROR(error, READ_FROM_LANG_FILE_ERROR,    "Error in reading file process!\n");
    PRINT_ERROR(error, DTOR_LANG_BUFFER_ERROR,       "Error in buffer destructor!\n");
    PRINT_ERROR(error, WRONG_LANG_SYNTAX,            "Wrong lang syntax!\n");
    PRINT_ERROR(error, NAME_TABLE_ALLOC_MEMORY_ERROR, "Error in calloc, tryed to alloc memory for name table\n");

    #undef PRINT_ERROR
}