#ifndef LAND_ERRORS_H
#define LANG_ERRORS_H

enum langErrorCode
{
    NO_LANG_ERRORS                       = 0,        
    LANG_FILE_OPEN_ERROR                 = 1 << 0,
    READ_FROM_LANG_FILE_ERROR            = 1 << 1,
    DTOR_LANG_BUFFER_ERROR               = 1 << 2,
    WRONG_LANG_SYNTAX                    = 1 << 3,
    NAME_TABLE_ALLOC_MEMORY_ERROR        = 1 << 4,
    BAD_NAME_TABLE                       = 1 << 5,
    UNDEFINED_NAME                       = 1 << 6,
    NAME_TABLE_DUMP_ERROR                = 1 << 7,
    LINE_ARRAY_ALLOC_ERROR               = 1 << 8,
    FUNCTION_REDECLARATION_ERROR         = 1 << 9,
    ID_EXPECTED                          = 1 << 10,
    KEYWORD_EXPECTED                     = 1 << 11,
    WRONG_KEY_WORD                       = 1 << 12,
    VARIABLE_REDECLARATION_ERROR         = 1 << 13,
    PROCESSOR_MEMORY_OVER                = 1 << 14,
    REPEARED_DECLARATION                 = 1 << 15
};

void print_lang_error(FILE* stream, langErrorCode error, size_t line, const char* text = nullptr);

#endif