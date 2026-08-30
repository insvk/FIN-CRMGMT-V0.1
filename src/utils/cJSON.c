/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
*/

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>

#include "cJSON.h"

static struct internal_hooks {
    void *(*allocate)(size_t size);
    void (*deallocate)(void *pointer);
} global_hooks = { malloc, free };

static unsigned char* cJSON_strdup(const unsigned char* string, const struct internal_hooks * const hooks) {
    size_t length = 0;
    unsigned char *copy = NULL;

    if (string == NULL) return NULL;
    length = strlen((const char*)string) + sizeof("");
    copy = (unsigned char*)hooks->allocate(length);
    if (copy == NULL) return NULL;
    memcpy(copy, string, length);
    return copy;
}

void cJSON_InitHooks(cJSON_Hooks* hooks) {
    if (!hooks) {
        global_hooks.allocate = malloc;
        global_hooks.deallocate = free;
        return;
    }
    global_hooks.allocate = (hooks->malloc_fn != NULL) ? hooks->malloc_fn : malloc;
    global_hooks.deallocate = (hooks->free_fn != NULL) ? hooks->free_fn : free;
}

static cJSON *cJSON_New_Item(const struct internal_hooks * const hooks) {
    cJSON* node = (cJSON*)hooks->allocate(sizeof(cJSON));
    if (node) memset(node, '\0', sizeof(cJSON));
    return node;
}

void cJSON_Delete(cJSON *c) {
    cJSON *next = NULL;
    while (c != NULL) {
        next = c->next;
        if (!(c->type & cJSON_IsReference) && (c->child != NULL)) {
            cJSON_Delete(c->child);
        }
        if (!(c->type & cJSON_IsReference) && (c->valuestring != NULL)) {
            global_hooks.deallocate(c->valuestring);
        }
        if (!(c->type & cJSON_StringIsConst) && (c->string != NULL)) {
            global_hooks.deallocate(c->string);
        }
        global_hooks.deallocate(c);
        c = next;
    }
}

typedef struct {
    const unsigned char *content;
    size_t length;
    size_t offset;
    size_t depth;
    struct internal_hooks hooks;
} parse_buffer;

#define can_read(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
#define cannot_read_num(buffer, size) (!can_read(buffer, size))
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)

static parse_buffer *skip_whitespace(parse_buffer * const buffer) {
    if ((buffer == NULL) || (buffer->content == NULL)) return NULL;
    while (can_read(buffer, 1) && (buffer_at_offset(buffer)[0] <= 32)) {
        buffer->offset++;
    }
    if (buffer->offset >= buffer->length) buffer->offset = buffer->length;
    return buffer;
}

static int parse_string(cJSON * const item, parse_buffer * const buffer) {
    const unsigned char *input_pointer = buffer_at_offset(buffer) + 1;
    const unsigned char *input_end = buffer_at_offset(buffer) + 1;
    unsigned char *output_pointer = NULL;
    unsigned char *output = NULL;
    size_t allocation_length = 0;
    size_t skipped_bytes = 0;

    if (buffer_at_offset(buffer)[0] != '\"') return 0;

    while (((size_t)(input_end - buffer->content) < buffer->length) && (*input_end != '\"')) {
        if (input_end[0] == '\\') {
            skipped_bytes++;
            input_end++;
        }
        input_end++;
    }
    if (((size_t)(input_end - buffer->content) >= buffer->length) || (*input_end != '\"')) return 0;

    allocation_length = (size_t)(input_end - buffer_at_offset(buffer)) - skipped_bytes;
    output = (unsigned char*)buffer->hooks.allocate(allocation_length + sizeof(""));
    if (output == NULL) return 0;

    output_pointer = output;
    while (input_pointer < input_end) {
        if (*input_pointer != '\\') {
            *output_pointer++ = *input_pointer++;
        } else {
            input_pointer++;
            switch (*input_pointer) {
                case 'b': *output_pointer++ = '\b'; break;
                case 'f': *output_pointer++ = '\f'; break;
                case 'n': *output_pointer++ = '\n'; break;
                case 'r': *output_pointer++ = '\r'; break;
                case 't': *output_pointer++ = '\t'; break;
                case '\"':
                case '\\':
                case '/': *output_pointer++ = *input_pointer; break;
                default: *output_pointer++ = *input_pointer; break;
            }
            input_pointer++;
        }
    }
    *output_pointer = '\0';
    item->type = cJSON_String;
    item->valuestring = (char*)output;
    buffer->offset = (size_t)(input_end - buffer->content) + 1;
    return 1;
}

static int parse_number(cJSON * const item, parse_buffer * const buffer) {
    double number = 0;
    unsigned char *after_end = NULL;
    const unsigned char *str = buffer_at_offset(buffer);

    if (str == NULL) return 0;
    number = strtod((const char*)str, (char**)&after_end);
    if (str == after_end) return 0;

    item->valuedouble = number;
    if (number >= INT_MAX) item->valueint = INT_MAX;
    else if (number <= (double)INT_MIN) item->valueint = INT_MIN;
    else item->valueint = (int)number;

    item->type = cJSON_Number;
    buffer->offset += (size_t)(after_end - str);
    return 1;
}

static int parse_value(cJSON * const item, parse_buffer * const buffer);

static int parse_array(cJSON * const item, parse_buffer * const buffer) {
    cJSON *head = NULL;
    cJSON *current_item = NULL;

    if (buffer->depth >= 1000) return 0;
    buffer->depth++;
    if (buffer_at_offset(buffer)[0] != '[') return 0;
    buffer->offset++;
    skip_whitespace(buffer);

    if (can_read(buffer, 1) && (buffer_at_offset(buffer)[0] == ']')) {
        buffer->offset++;
        item->type = cJSON_Array;
        buffer->depth--;
        return 1;
    }

    while (can_read(buffer, 1)) {
        cJSON *new_item = cJSON_New_Item(&(buffer->hooks));
        if (new_item == NULL) goto fail;
        if (head == NULL) {
            current_item = head = new_item;
        } else {
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        if (!parse_value(new_item, buffer)) goto fail;
        skip_whitespace(buffer);

        if (can_read(buffer, 1) && (buffer_at_offset(buffer)[0] == ',')) {
            buffer->offset++;
            skip_whitespace(buffer);
        } else if (can_read(buffer, 1) && (buffer_at_offset(buffer)[0] == ']')) {
            buffer->offset++;
            item->type = cJSON_Array;
            item->child = head;
            buffer->depth--;
            return 1;
        } else goto fail;
    }

fail:
    if (head != NULL) cJSON_Delete(head);
    return 0;
}

static int parse_object(cJSON * const item, parse_buffer * const buffer) {
    cJSON *head = NULL;
    cJSON *current_item = NULL;

    if (buffer->depth >= 1000) return 0;
    buffer->depth++;
    if (buffer_at_offset(buffer)[0] != '{') return 0;
    buffer->offset++;
    skip_whitespace(buffer);

    if (can_read(buffer, 1) && (buffer_at_offset(buffer)[0] == '}')) {
        buffer->offset++;
        item->type = cJSON_Object;
        buffer->depth--;
        return 1;
    }

    while (can_read(buffer, 1)) {
        cJSON *new_item = cJSON_New_Item(&(buffer->hooks));
        if (new_item == NULL) goto fail;
        if (head == NULL) {
            current_item = head = new_item;
        } else {
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }

        if (!parse_string(new_item, buffer)) goto fail;
        new_item->string = new_item->valuestring;
        new_item->valuestring = NULL;

        skip_whitespace(buffer);
        if (cannot_read_num(buffer, 1) || (buffer_at_offset(buffer)[0] != ':')) goto fail;
        buffer->offset++;
        skip_whitespace(buffer);

        if (!parse_value(new_item, buffer)) goto fail;
        skip_whitespace(buffer);

        if (can_read(buffer, 1) && (buffer_at_offset(buffer)[0] == ',')) {
            buffer->offset++;
            skip_whitespace(buffer);
        } else if (can_read(buffer, 1) && (buffer_at_offset(buffer)[0] == '}')) {
            buffer->offset++;
            item->type = cJSON_Object;
            item->child = head;
            buffer->depth--;
            return 1;
        } else goto fail;
    }

fail:
    if (head != NULL) cJSON_Delete(head);
    return 0;
}

static int parse_value(cJSON * const item, parse_buffer * const buffer) {
    if ((buffer == NULL) || (buffer->content == NULL)) return 0;
    skip_whitespace(buffer);
    if (!can_read(buffer, 1)) return 0;

    switch (buffer_at_offset(buffer)[0]) {
        case 'n':
            if (can_read(buffer, 4) && (strncmp((const char*)buffer_at_offset(buffer), "null", 4) == 0)) {
                item->type = cJSON_NULL;
                buffer->offset += 4;
                return 1;
            }
            break;
        case 't':
            if (can_read(buffer, 4) && (strncmp((const char*)buffer_at_offset(buffer), "true", 4) == 0)) {
                item->type = cJSON_True;
                item->valueint = 1;
                buffer->offset += 4;
                return 1;
            }
            break;
        case 'f':
            if (can_read(buffer, 5) && (strncmp((const char*)buffer_at_offset(buffer), "false", 5) == 0)) {
                item->type = cJSON_False;
                item->valueint = 0;
                buffer->offset += 5;
                return 1;
            }
            break;
        case '\"':
            return parse_string(item, buffer);
        case '[':
            return parse_array(item, buffer);
        case '{':
            return parse_object(item, buffer);
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number(item, buffer);
        default:
            break;
    }
    return 0;
}

cJSON *cJSON_ParseWithLength(const char *value, size_t buffer_length) {
    parse_buffer buffer = { 0, 0, 0, 0, { 0, 0 } };
    cJSON *item = NULL;

    if (value == NULL || buffer_length == 0) return NULL;
    buffer.content = (const unsigned char*)value;
    buffer.length = buffer_length;
    buffer.hooks = global_hooks;

    item = cJSON_New_Item(&global_hooks);
    if (item == NULL) return NULL;

    if (!parse_value(item, &buffer)) {
        cJSON_Delete(item);
        return NULL;
    }
    return item;
}

cJSON *cJSON_Parse(const char *value) {
    if (value == NULL) return NULL;
    return cJSON_ParseWithLength(value, strlen(value));
}

typedef struct {
    unsigned char *buffer;
    size_t length;
    size_t offset;
    size_t depth;
    int noalloc;
    int format;
    struct internal_hooks hooks;
} printbuffer;

static unsigned char* ensure(printbuffer * const p, size_t needed) {
    unsigned char *newbuffer = NULL;
    size_t newsize = 0;

    if ((p == NULL) || (p->buffer == NULL)) return NULL;
    if ((p->length > 0) && (p->offset >= p->length)) return NULL;

    if (needed > INT_MAX) return NULL;
    needed += p->offset + 1;
    if (needed <= p->length) return p->buffer + p->offset;

    if (p->noalloc) return NULL;

    newsize = needed * 2;
    newbuffer = (unsigned char*)p->hooks.allocate(newsize);
    if (newbuffer == NULL) return NULL;

    memcpy(newbuffer, p->buffer, p->length);
    p->hooks.deallocate(p->buffer);
    p->length = newsize;
    p->buffer = newbuffer;
    return newbuffer + p->offset;
}

static int print_value(const cJSON * const item, printbuffer * const output_buffer);

static int print_string(const unsigned char * const input, printbuffer * const output_buffer) {
    size_t length = 0;
    unsigned char *output = NULL;
    if (input == NULL) {
        output = ensure(output_buffer, 2);
        if (!output) return 0;
        strcpy((char*)output, "\"\"");
        output_buffer->offset += 2;
        return 1;
    }
    length = strlen((const char*)input);
    output = ensure(output_buffer, length + 2 + 16);
    if (!output) return 0;
    *output++ = '\"';
    output_buffer->offset++;
    for (size_t i = 0; i < length; i++) {
        if (input[i] == '\"' || input[i] == '\\') {
            output = ensure(output_buffer, 2);
            if (!output) return 0;
            *output++ = '\\';
            *output++ = input[i];
            output_buffer->offset += 2;
        } else if (input[i] == '\n') {
            output = ensure(output_buffer, 2);
            if (!output) return 0;
            *output++ = '\\'; *output++ = 'n';
            output_buffer->offset += 2;
        } else if (input[i] == '\r') {
            output = ensure(output_buffer, 2);
            if (!output) return 0;
            *output++ = '\\'; *output++ = 'r';
            output_buffer->offset += 2;
        } else if (input[i] == '\t') {
            output = ensure(output_buffer, 2);
            if (!output) return 0;
            *output++ = '\\'; *output++ = 't';
            output_buffer->offset += 2;
        } else {
            output = ensure(output_buffer, 1);
            if (!output) return 0;
            *output++ = input[i];
            output_buffer->offset++;
        }
    }
    output = ensure(output_buffer, 1);
    if (!output) return 0;
    *output++ = '\"';
    output_buffer->offset++;
    return 1;
}

static int print_number(const cJSON * const item, printbuffer * const output_buffer) {
    unsigned char *output = NULL;
    double d = item->valuedouble;
    char number_buffer[64];
    int length = 0;

    if (fabs(((double)item->valueint) - d) <= DBL_EPSILON && d <= INT_MAX && d >= INT_MIN) {
        length = snprintf(number_buffer, sizeof(number_buffer), "%d", item->valueint);
    } else {
        length = snprintf(number_buffer, sizeof(number_buffer), "%1.15g", d);
    }
    if (length < 0) return 0;

    output = ensure(output_buffer, (size_t)length);
    if (!output) return 0;
    memcpy(output, number_buffer, (size_t)length);
    output_buffer->offset += (size_t)length;
    return 1;
}

static int print_array(const cJSON * const item, printbuffer * const output_buffer) {
    unsigned char *output = NULL;
    cJSON *current_element = item->child;

    output = ensure(output_buffer, 1);
    if (!output) return 0;
    *output = '[';
    output_buffer->offset++;

    while (current_element != NULL) {
        if (!print_value(current_element, output_buffer)) return 0;
        if (current_element->next) {
            output = ensure(output_buffer, 1);
            if (!output) return 0;
            *output = ',';
            output_buffer->offset++;
        }
        current_element = current_element->next;
    }
    output = ensure(output_buffer, 1);
    if (!output) return 0;
    *output = ']';
    output_buffer->offset++;
    return 1;
}

static int print_object(const cJSON * const item, printbuffer * const output_buffer) {
    unsigned char *output = NULL;
    cJSON *current_item = item->child;

    output = ensure(output_buffer, 1);
    if (!output) return 0;
    *output = '{';
    output_buffer->offset++;

    while (current_item != NULL) {
        if (!print_string((unsigned char*)current_item->string, output_buffer)) return 0;
        output = ensure(output_buffer, 1);
        if (!output) return 0;
        *output = ':';
        output_buffer->offset++;

        if (!print_value(current_item, output_buffer)) return 0;
        if (current_item->next) {
            output = ensure(output_buffer, 1);
            if (!output) return 0;
            *output = ',';
            output_buffer->offset++;
        }
        current_item = current_item->next;
    }
    output = ensure(output_buffer, 1);
    if (!output) return 0;
    *output = '}';
    output_buffer->offset++;
    return 1;
}

static int print_value(const cJSON * const item, printbuffer * const output_buffer) {
    unsigned char *output = NULL;
    if (item == NULL) return 0;

    switch ((item->type) & 0xFF) {
        case cJSON_NULL:
            output = ensure(output_buffer, 4);
            if (!output) return 0;
            memcpy(output, "null", 4);
            output_buffer->offset += 4;
            return 1;
        case cJSON_False:
            output = ensure(output_buffer, 5);
            if (!output) return 0;
            memcpy(output, "false", 5);
            output_buffer->offset += 5;
            return 1;
        case cJSON_True:
            output = ensure(output_buffer, 4);
            if (!output) return 0;
            memcpy(output, "true", 4);
            output_buffer->offset += 4;
            return 1;
        case cJSON_Number:
            return print_number(item, output_buffer);
        case cJSON_String:
            return print_string((unsigned char*)item->valuestring, output_buffer);
        case cJSON_Array:
            return print_array(item, output_buffer);
        case cJSON_Object:
            return print_object(item, output_buffer);
        case cJSON_Raw:
            if (item->valuestring == NULL) return 0;
            size_t raw_len = strlen(item->valuestring);
            output = ensure(output_buffer, raw_len);
            if (!output) return 0;
            memcpy(output, item->valuestring, raw_len);
            output_buffer->offset += raw_len;
            return 1;
        default:
            return 0;
    }
}

char *cJSON_PrintUnformatted(const cJSON *item) {
    printbuffer buffer = { 0, 0, 0, 0, 0, 0, { 0, 0 } };
    buffer.hooks = global_hooks;
    buffer.buffer = (unsigned char*)buffer.hooks.allocate(256);
    if (!buffer.buffer) return NULL;
    buffer.length = 256;

    if (!print_value(item, &buffer)) {
        buffer.hooks.deallocate(buffer.buffer);
        return NULL;
    }
    unsigned char *out = ensure(&buffer, 1);
    if (!out) {
        buffer.hooks.deallocate(buffer.buffer);
        return NULL;
    }
    *out = '\0';
    return (char*)buffer.buffer;
}

char *cJSON_Print(const cJSON *item) {
    return cJSON_PrintUnformatted(item);
}

int cJSON_GetArraySize(const cJSON *array) {
    cJSON *child = NULL;
    size_t size = 0;
    if (array == NULL) return 0;
    child = array->child;
    while (child != NULL) {
        size++;
        child = child->next;
    }
    return (int)size;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int index) {
    cJSON *current_child = NULL;
    if (array == NULL || index < 0) return NULL;
    current_child = array->child;
    while ((current_child != NULL) && (index > 0)) {
        index--;
        current_child = current_child->next;
    }
    return current_child;
}

cJSON *cJSON_GetObjectItem(const cJSON * const object, const char * const string) {
    cJSON *current_element = NULL;
    if (object == NULL || string == NULL) return NULL;
    current_element = object->child;
    while (current_element != NULL) {
        if (current_element->string && strcmp(string, current_element->string) == 0) {
            return current_element;
        }
        current_element = current_element->next;
    }
    return NULL;
}

cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON * const object, const char * const string) {
    return cJSON_GetObjectItem(object, string);
}

int cJSON_HasObjectItem(const cJSON *object, const char *string) {
    return cJSON_GetObjectItem(object, string) != NULL;
}

cJSON *cJSON_CreateNull(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) item->type = cJSON_NULL;
    return item;
}

cJSON *cJSON_CreateTrue(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) { item->type = cJSON_True; item->valueint = 1; }
    return item;
}

cJSON *cJSON_CreateFalse(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) { item->type = cJSON_False; item->valueint = 0; }
    return item;
}

cJSON *cJSON_CreateBool(int b) {
    return b ? cJSON_CreateTrue() : cJSON_CreateFalse();
}

cJSON *cJSON_CreateNumber(double num) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Number;
        item->valuedouble = num;
        item->valueint = (int)num;
    }
    return item;
}

cJSON *cJSON_CreateString(const char *string) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_String;
        item->valuestring = (char*)cJSON_strdup((const unsigned char*)string, &global_hooks);
    }
    return item;
}

cJSON *cJSON_CreateRaw(const char *raw) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Raw;
        item->valuestring = (char*)cJSON_strdup((const unsigned char*)raw, &global_hooks);
    }
    return item;
}

cJSON *cJSON_CreateArray(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) item->type = cJSON_Array;
    return item;
}

cJSON *cJSON_CreateObject(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) item->type = cJSON_Object;
    return item;
}

void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
    cJSON *child = NULL;
    if (item == NULL || array == NULL) return;
    child = array->child;
    if (child == NULL) {
        array->child = item;
    } else {
        while (child->next) child = child->next;
        child->next = item;
        item->prev = child;
    }
}

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
    if (item == NULL || object == NULL || string == NULL) return;
    if (item->string) global_hooks.deallocate(item->string);
    item->string = (char*)cJSON_strdup((const unsigned char*)string, &global_hooks);
    cJSON_AddItemToArray(object, item);
}

int cJSON_IsNull(const cJSON * const item) {
    return (item != NULL) && ((item->type & 0xFF) == cJSON_NULL);
}
int cJSON_IsBool(const cJSON * const item) {
    return (item != NULL) && (((item->type & 0xFF) == cJSON_True) || ((item->type & 0xFF) == cJSON_False));
}
int cJSON_IsTrue(const cJSON * const item) {
    return (item != NULL) && ((item->type & 0xFF) == cJSON_True);
}
int cJSON_IsFalse(const cJSON * const item) {
    return (item != NULL) && ((item->type & 0xFF) == cJSON_False);
}
int cJSON_IsNumber(const cJSON * const item) {
    return (item != NULL) && ((item->type & 0xFF) == cJSON_Number);
}
int cJSON_IsString(const cJSON * const item) {
    return (item != NULL) && ((item->type & 0xFF) == cJSON_String);
}
int cJSON_IsArray(const cJSON * const item) {
    return (item != NULL) && ((item->type & 0xFF) == cJSON_Array);
}
int cJSON_IsObject(const cJSON * const item) {
    return (item != NULL) && ((item->type & 0xFF) == cJSON_Object);
}
