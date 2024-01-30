#include <stdio.h>
#include "assert.h"

#include "Tree.h"
#include "DataBuffer.h"
#include "LangErrors.h"
#include "NameTable.h"
#include "Output.h"

static langErrorCode write_lang_tree_to_buffer_recursive(outputBuffer* buffer, const TreeSegment* segment);

static langErrorCode write_name_table_to_file(outputBuffer* buffer, const LangNameTable* table);

langErrorCode write_lang_tree_to_file(const char* filename, const TreeData* tree)
{
    assert(filename);
    assert(tree);

    outputBuffer buffer = {.AUTO_FLUSH = true};

    langErrorCode error = NO_LANG_ERRORS;

    create_output_file(&buffer.filePointer, filename, TEXT);

    if ((error = write_lang_tree_to_buffer_recursive(&buffer, tree->root)))
    {
        return error;
    }
    
    write_buffer_to_file(&buffer);

    fclose(buffer.filePointer);
    
    return error;
}

static langErrorCode write_lang_tree_to_buffer_recursive(outputBuffer* buffer, const TreeSegment* segment)
{
    assert(buffer);
    assert(segment);

    print_to_buffer(buffer, "( ");

    switch((int) segment->type)
    {
        case TEXT_SEGMENT_DATA:
            print_to_buffer(buffer, "\"%s\" ", segment->data.stringPtr);
            break;
        case DOUBLE_SEGMENT_DATA:
            print_to_buffer(buffer, "2 %lf ", segment->data.D_number);
            break;
        case IDENTIFIER:
            print_to_buffer(buffer, "5 %lu ", segment->data.Id);
            break;
        case KEYWORD:
            print_to_buffer(buffer, "6 %d ", segment->data.K_word);
            break;
        case FUNCTION_DEFINITION:
            print_to_buffer(buffer, "7 %lu ", segment->data.Id);
            break;
        case PARAMETERS:
            print_to_buffer(buffer, "8 0 ");
            break;
        case VAR_DECLARATION:
            print_to_buffer(buffer, "9 %lu ", segment->data.Id);
            break;
        case CALL:
            print_to_buffer(buffer, "10 0 ");
            break;
        case NO_TYPE_SEGMENT_DATA:
            print_to_buffer(buffer, "NONE ");
            break;

        default:
            break;
    }

    if (segment->left)
    {
        write_lang_tree_to_buffer_recursive(buffer, segment->left);
        print_to_buffer(buffer, " ");
    }
    else
    {
        print_to_buffer(buffer, "_ ");
    }

    if (segment->right)
    {
        write_lang_tree_to_buffer_recursive(buffer, segment->right);
        print_to_buffer(buffer, " ");
    }
    else
    {
        print_to_buffer(buffer, "_ ");
    }

    print_to_buffer(buffer, ")");

    return NO_LANG_ERRORS;
}

langErrorCode write_name_table_array_to_file(const char* filename, const LangNameTableArray* table_array)
{
    assert(table_array);

    outputBuffer buffer = {.AUTO_FLUSH = true};
    langErrorCode error = NO_LANG_ERRORS;

    create_output_file(&buffer.filePointer, filename, TEXT);

    for (size_t i = 0; i <= table_array->size; i++)
    {
        if ((error = write_name_table_to_file(&buffer, &(table_array->Array[i]))))
        {
            return error;
        }
    }

    write_buffer_to_file(&buffer);

    fclose(buffer.filePointer);

    return error;
}

static langErrorCode write_name_table_to_file(outputBuffer* buffer, const LangNameTable* table)
{
    assert(buffer);
    assert(table);

    print_to_buffer(buffer, "( %lu ", table->table_number);

    for (size_t i = 0; i < table->Pointer; i++)
    {
        print_to_buffer(buffer, "( %s %lu %lu ) ", table->Table[i].name, table->Table[i].number, table->Table[i].type);
    }

    print_to_buffer(buffer, ") ");

    return NO_LANG_ERRORS;
}