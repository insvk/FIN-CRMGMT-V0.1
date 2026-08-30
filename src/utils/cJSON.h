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

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

#define cJSON_Invalid (0)
#define cJSON_False  (1 << 0)
#define cJSON_True   (1 << 1)
#define cJSON_NULL   (1 << 2)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
#define cJSON_Array  (1 << 5)
#define cJSON_Object (1 << 6)
#define cJSON_Raw    (1 << 7)

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

typedef struct cJSON
{
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;

typedef struct cJSON_Hooks
{
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} cJSON_Hooks;

#define cJSON_SetIntValue(object, val) ((object)?(object)->valueint = (object)->valuedouble = (val):(val))
#define cJSON_SetNumberValue(object, val) ((object)?(object)->valueint = (int)((object)->valuedouble = (val)):(val))

const char *cJSON_Version(void);
void cJSON_InitHooks(cJSON_Hooks* hooks);

cJSON *cJSON_Parse(const char *value);
cJSON *cJSON_ParseWithLength(const char *value, size_t buffer_length);
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated);

char *cJSON_Print(const cJSON *item);
char *cJSON_PrintUnformatted(const cJSON *item);
char *cJSON_PrintBuffered(const cJSON *item, int prebuffer, int fmt);
void cJSON_Delete(cJSON *c);

int cJSON_GetArraySize(const cJSON *array);
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);
cJSON *cJSON_GetObjectItem(const cJSON * const object, const char * const string);
cJSON *cJSON_GetObjectItemCaseSensitive(const cJSON * const object, const char * const string);
int cJSON_HasObjectItem(const cJSON *object, const char *string);

cJSON *cJSON_CreateNull(void);
cJSON *cJSON_CreateTrue(void);
cJSON *cJSON_CreateFalse(void);
cJSON *cJSON_CreateBool(int b);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateString(const char *string);
cJSON *cJSON_CreateRaw(const char *raw);
cJSON *cJSON_CreateArray(void);
cJSON *cJSON_CreateObject(void);

cJSON *cJSON_CreateIntArray(const int *numbers, int count);
cJSON *cJSON_CreateFloatArray(const float *numbers, int count);
cJSON *cJSON_CreateDoubleArray(const double *numbers, int count);
cJSON *cJSON_CreateStringArray(const char **strings, int count);

void cJSON_AddItemToArray(cJSON *array, cJSON *item);
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
void cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item);

cJSON *cJSON_DetachItemViaPointer(cJSON *parent, cJSON * const item);
cJSON *cJSON_DetachItemFromArray(cJSON *array, int which);
void cJSON_DeleteItemFromArray(cJSON *array, int which);
cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string);
cJSON *cJSON_DetachItemFromObjectCaseSensitive(cJSON *object, const char *string);
void cJSON_DeleteItemFromObject(cJSON *object, const char *string);
void cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string);

cJSON *cJSON_Duplicate(const cJSON *item, int recurse);
int cJSON_Compare(const cJSON * const a, const cJSON * const b, const int case_sensitive);

int cJSON_IsInvalid(const cJSON * const item);
int cJSON_IsFalse(const cJSON * const item);
int cJSON_IsTrue(const cJSON * const item);
int cJSON_IsBool(const cJSON * const item);
int cJSON_IsNull(const cJSON * const item);
int cJSON_IsNumber(const cJSON * const item);
int cJSON_IsString(const cJSON * const item);
int cJSON_IsArray(const cJSON * const item);
int cJSON_IsObject(const cJSON * const item);
int cJSON_IsRaw(const cJSON * const item);

#define cJSON_AddNullToObject(object, name) cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object, name) cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object, name) cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object, name, b) cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object, name, n) cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object, name, s) cJSON_AddItemToObject(object, name, cJSON_CreateString(s))
#define cJSON_AddRawToObject(object, name, s) cJSON_AddItemToObject(object, name, cJSON_CreateRaw(s))
#define cJSON_AddObjectToObject(object, name) cJSON_AddItemToObject(object, name, cJSON_CreateObject())
#define cJSON_AddArrayToObject(object, name) cJSON_AddItemToObject(object, name, cJSON_CreateArray())

#ifdef __cplusplus
}
#endif

#endif
