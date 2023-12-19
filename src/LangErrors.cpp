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

    PRINT_ERROR(error, LANG_FILE_OPEN_ERROR,        "Error in open file process!\n");

    #undef PRINT_ERROR
}