#include <stdio.h>

#include "Tree.h"
#include "LangErrors.h"
#include "FrontEnd.h"
#include "BackEnd.h"
#include "Output.h"

int main()
{
    TreeData tree = {};
    LangNameTableArray table = {};
    langErrorCode error = NO_LANG_ERRORS;
    
    if ((error = name_table_array_ctor(&table)))
    {
        print_lang_error(stderr, error, 0);
    }

    if ((error = lang_parser("text.txt", &tree, &table)))
    {
        print_lang_error(stderr, error, 0);
    }

    // Запись дерева в выходной файл
    write_lang_tree_to_file("out.txt", &tree);
    // Запись таблиц имён в выходной вайл
    write_name_table_array_to_file("table_out.txt", &table);

    TreeData new_tree = {};
    
    if ((error = tree_reader("out.txt", &new_tree)))
    {
        print_lang_error(stderr, error, 0);
    }

    tree_dump(&new_tree);
    name_table_array_dump(&table);
    name_table_array_dtor(&table);
    tree_dtor(&tree);
    tree_dtor(&new_tree);
    return 0;
}