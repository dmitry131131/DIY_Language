#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <math.h>
#include <sys/stat.h>

#include "Tree.h"
#include "LangErrors.h"
#include "DataBuffer.h"
#include "TreeReader.h"

static TreeSegment* read_lang_tree_from_file(outputBuffer* buffer, TreeSegment* par_segment, langErrorCode* error);
static langErrorCode add_value_to_segment(TreeSegment* segment, size_t value);

// TODO добавить новые ошибки
langErrorCode tree_reader(const char* filename, TreeData* tree)
{
    assert(filename);
    assert(tree);

    langErrorCode error = NO_LANG_ERRORS;

    FILE* file = fopen(filename, "r");

    if (!file)
    {
        return LANG_FILE_OPEN_ERROR;
    }

    struct stat buff = {};
    if (fstat(fileno(file), &buff))
    {
        return LANG_FILE_OPEN_ERROR;
    }
    if (buff.st_size < 0) return LANG_FILE_OPEN_ERROR;

    outputBuffer treeBuffer;

    if (buffer_ctor(&treeBuffer, (size_t) buff.st_size))
    {
        return LANG_FILE_OPEN_ERROR;
    }

    if (fread(treeBuffer.customBuffer, sizeof(char), treeBuffer.customSize, file) != treeBuffer.customSize)
    {
        return LANG_FILE_OPEN_ERROR;
    }

    if (tree->root)
    {
        return error;
    }

    tree->root = read_lang_tree_from_file(&treeBuffer, nullptr, &error);

    buffer_dtor(&treeBuffer);
    fclose(file);

    return error;
}

static TreeSegment* read_lang_tree_from_file(outputBuffer* buffer, TreeSegment* par_segment, langErrorCode* error)
{
    assert(buffer);

    TreeSegment* seg;
    
    if (((char) buffer->customBuffer[buffer->bufferPointer]) == '(')
    {
        (buffer->bufferPointer) += 2;

        size_t segment_type = 0;
        int    symbol_count = 0;
        
        sscanf(buffer->customBuffer + buffer->bufferPointer, "%lu%n", &segment_type, &symbol_count);
        buffer->bufferPointer += (size_t) symbol_count;

        seg = new_segment((SegmemtType) segment_type, sizeof(size_t), par_segment);

        if (segment_type == (size_t) DOUBLE_SEGMENT_DATA)
        {
            double seg_data = NAN;
            sscanf(buffer->customBuffer + buffer->bufferPointer, "%lf%n", &seg_data, &symbol_count);
            buffer->bufferPointer += (size_t) symbol_count;
            seg->data.D_number = seg_data;
        }
        else
        {
            size_t seg_data = 0;
            sscanf(buffer->customBuffer + buffer->bufferPointer, "%lu%n", &seg_data, &symbol_count);
            buffer->bufferPointer += (size_t) symbol_count;
            add_value_to_segment(seg, seg_data);
        }
    }
    else
    {
        //error
    }

    (buffer->bufferPointer)++;

    if (((char) buffer->customBuffer[buffer->bufferPointer]) == '(')
    {
        seg->left = read_lang_tree_from_file(buffer, seg, error);
    }
    else if (((char) buffer->customBuffer[buffer->bufferPointer]) == '_')
    {
        seg->left = NULL;
        (buffer->bufferPointer)++;
    }
    else
    {
        if (error) *error = LANG_FILE_OPEN_ERROR;
        del_segment(seg);
        return NULL;
    }

    (buffer->bufferPointer)++;
    
    if (((char) buffer->customBuffer[buffer->bufferPointer]) == '(')
    {
        seg->right = read_lang_tree_from_file(buffer, seg, error);
    }
    else if (((char) buffer->customBuffer[buffer->bufferPointer]) == '_')
    {
        seg->right = NULL;
        (buffer->bufferPointer)++;
    }
    else
    {
        if (error) *error = LANG_FILE_OPEN_ERROR;
        del_segment(seg);
        return NULL;
    }

    (buffer->bufferPointer)++;
    
    if (((char) buffer->customBuffer[buffer->bufferPointer]) == ')')
    {
        (buffer->bufferPointer)++;
        return seg;
    }
    else
    {
        if (error) *error = LANG_FILE_OPEN_ERROR;
        del_segment(seg);
        return NULL;
    }

    return seg;
}

static langErrorCode add_value_to_segment(TreeSegment* segment, size_t value)
{
    assert(segment);

    switch ((size_t) segment->type)
    {
    case IDENTIFIER:
        segment->data.Id = value;
        break;
    case KEYWORD:
        segment->data.K_word = (KeyWords) value;
        break;
    case FUNCTION_DEFINITION:
        segment->data.Id = value;
        break;
    case VAR_DECLARATION:
        segment->data.Id = value;
        break;
    case CALL:
        segment->data.Id = value;
        break;
    case PARAMETERS:
        segment->data.Id = value;
        break;
    
    default:
        break;
    }

    return NO_LANG_ERRORS;
}