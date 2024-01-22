#include <stdio.h>

#include "Tree.h"
#include "LangErrors.h"
#include "FrontEnd.h"
int main()
{
    TreeData tree = {};
    LangNameTableArray table = {};
    langErrorCode error = NO_LANG_ERRORS;
    
    if ((error = name_table_array_ctor(&table)))
    {
        print_lang_error(stderr, error);
    }

    if ((error = lang_parser("text.txt", &tree, &table)))
    {
        print_lang_error(stderr, error);
    }

    tree_dump(&tree);
    name_table_array_dump(&table);
    name_table_array_dtor(&table);
    tree_dtor(&tree);
    return 0;
}