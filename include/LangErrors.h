#ifndef LAND_ERRORS_H
#define LANG_ERRORS_H

enum langErrorCode
{
    NO_LANG_ERRORS                       = 0,        
    LANG_FILE_OPEN_ERROR                 = 1 << 0,
    READ_FROM_LANG_FILE_ERROR            = 1 << 1,
    DTOR_LANG_BUFFER_ERROR               = 1 << 2,
    WRONG_LANG_SYNTAX                    = 1 << 3
};

void print_lang_error(FILE* stream, langErrorCode error);

#endif