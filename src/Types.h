#pragma once

#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include "Macros.h"

#define LIST_CAPACITY_BATCH 3

typedef unsigned char byte;

typedef enum {
  CCFG_TK_NULL_TOKEN,
  CCFG_TK_LITERAL,
  CCFG_TK_DOT,
  CCFG_TK_ASSIGN,
  CCFG_TK_IDENTIFIER,
  CCFG_TK_OPEN_SQUARE,
  CCFG_TK_CLOSE_SQUARE,
  CCFG_TK_OPEN_CURLY,
  CCFG_TK_CLOSE_CURLY,
  CCFG_TK_COMMA
} TokenType;

typedef struct {
  TokenType type;
  char* value;
  long line;
} Token;

typedef struct {
  void* elements;
  size_t element_size;
  size_t size;
  size_t capacity;
} List;

typedef enum {
  CCFG_NULL,
  CCFG_INTEGER,
  CCFG_DOUBLE,
  CCFG_STRING,
  CCFG_BOOLEAN,
  CCFG_ARRAY,
  CCFG_TABLE
} CCFG_ValueType;

struct ccfg_object;

typedef struct {
  char** keys;
  struct ccfg_object* values;
  size_t size;
  size_t capacity;
} CCFG_Table;

typedef union {
  long i;
  double d;
  char* s;
  bool b;
  List* arr;
  CCFG_Table* tbl;
} CCFG_Data;

typedef struct ccfg_object {
  CCFG_ValueType type;
  CCFG_Data value;
} CCFG_Object;

typedef struct {
  const char* name;
  char* content;
  size_t index;
  size_t size;
} FileDescriptor;

typedef struct {
  Token* content;
  size_t index;
  size_t size;
} TokenStream;

typedef struct {
  char* cstr;
  size_t size;
} string;

typedef struct {
  bool isPresent;
  bool isNull;
  long toLong;
  double toDouble;
  char* toString;
  bool toBool;
} CCFG_Primitive;

struct ccfg_array;
struct ccfg_simpleTable;

typedef struct {
  bool isPresent;
  struct ccfg_array* toArray;
  struct ccfg_simpleTable* toTable;
} CCFG_Composite;

typedef union {
  CCFG_Primitive primitive;
  CCFG_Composite composite;
} CCFG_Any;

typedef struct ccfg_array {
  CCFG_Any* array;
  size_t size;
} CCFG_Array;

typedef struct ccfg_simpleTable {
  char** keys;
  CCFG_Any* values;
  size_t size;
} CCFG_SimpleTable;
