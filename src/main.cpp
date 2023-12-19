#include <stdio.h>

#include "Tree.h"
#include "LangErrors.h"
#include "FrontEnd.h"
int main()
{
    TreeData tree = {};
    
    lang_parser("text.txt", &tree);
    return 0;
}