#pragma once

#include <stdio.h>
#include <memory.h>
#include "Types.h"
#include <stdbool.h>
#include "Constants.h"
#include <string.h>
#include "Macros.h"

long CCFG_ceil(double d);
FileDescriptor CCFG_fopen(const char* fname);

char* listAsString(List* src);
List listOf(size_t element_size);
int listPush(List* dst, void* element);
int listPop(List* src);

bool _fileHasPeek(FileDescriptor* file, int amount);
#define fileHasPeek(file) _fileHasPeek(file, 0)
char _filePeek(FileDescriptor* file, int amount);
#define filePeek(file) _filePeek(file, 0)
char fileConsume(FileDescriptor* file);
bool fileTryConsume(FileDescriptor* file, char c);
void error(const char* error, int line);

void tokenize(List* output, FileDescriptor* file);
void printfToken(Token* token);
void printfTokens(List* tokens);

static const Token EMPTY_TOKEN = {
  .type = CCFG_TK_NULL_TOKEN,
  .line = -1,
  .value = nullptr
};

TokenStream createTokenStream(List* tokens);

bool _tokenStreamHasPeek(TokenStream* stream, int amount);
#define tokenStreamHasPeek(stream) _tokenStreamHasPeek(stream, 0)
Token _tokenStreamPeek(TokenStream* stream, int amount);
#define tokenStreamPeek(stream) _tokenStreamPeek(stream, 0)
Token tokenStreamConsume(TokenStream* stream);
bool _tokenStreamPeekEqual(TokenStream* stream, TokenType t, char* val);
bool _tokenStreamTryConsume(TokenStream* stream, TokenType t, char* val);
Token _tokenStreamTryConsumeError(TokenStream* stream, TokenType t, char* val, const char* err);
#define tokenStreamPeekEqual(stream, type) _tokenStreamPeekEqual(stream, type, nullptr)
#define tokenStreamTryConsume(stream, type) _tokenStreamTryConsume(stream, type, nullptr)
#define tokenStreamTryConsumeError(stream, type, err) _tokenStreamTryConsumeError(stream, type, nullptr, err)

CCFG_Table* newTable();
size_t tableContains(CCFG_Table* tbl, char* name);
int addToTable(CCFG_Table* tbl, char* name, CCFG_Object* value);

bool isNumber(char* s);
bool isFloat(char* s);

CCFG_Object* parseLiteral(char* lit);
CCFG_Object* parseValue(TokenStream* stream);
void parseObject(TokenStream* stream, CCFG_Table* parent);
CCFG_Object* parse(TokenStream* stream);

void printMultiple(const char* s, long amount);
void _printObject(CCFG_Object* obj, long depth);
#define printObject(obj) _printObject(obj, 0)

CCFG_Object* parseFile(const char* fname);

CCFG_Primitive getPrimitive(CCFG_Object* obj);
CCFG_Array* getArrayFromList(List* lst);
long stfind(CCFG_SimpleTable* st, char* key);
CCFG_Any stget(CCFG_SimpleTable* st, char* key);
CCFG_SimpleTable* getSTBLFromTable(CCFG_Table* tbl);
CCFG_Composite getComposite(CCFG_Object* obj);

CCFG_Any geti(CCFG_Object* obj, long i);
CCFG_Any getk(CCFG_Object* obj, char* key);
