#include <stdio.h>

#include "Tree.h"
#include "LangErrors.h"
#include "FrontEnd.h"
int main()
{
    TreeData tree = {};
    langErrorCode error = NO_LANG_ERRORS;
    
    if ((error = lang_parser("text.txt", &tree)))
    {
        print_lang_error(stderr, error);
    }

    tree_dump(&tree);
    tree_dtor(&tree);
    return 0;
}