#include <CCFG.h>

long CCFG_ceil(double d) {
  long i = (long) d;
  double decimal = d - i;
  return decimal > 0 ? ++i : i;
}

FileDescriptor CCFG_fopen(const char* fname) {
  FileDescriptor descriptor;
  FILE* f = fopen(fname, "r");
  
  descriptor.size = 1;
  char c;
  while (fread(&c, sizeof(char), 1, f) > 0)
    descriptor.size++;

  fseek(f, 0, SEEK_SET);

  descriptor.index = 0;
  descriptor.name = fname;
  descriptor.content = allocate(descriptor.size);

  const size_t BUF_SZ = 4096;

  byte buffer[BUF_SZ];
  size_t bytes_read;
  size_t size = 0;
  while ((bytes_read = fread(buffer, sizeof(char), BUF_SZ, f)) > 0) {
    memcpy(&(descriptor.content[size]), buffer, bytes_read);
    size += bytes_read;
  }
  descriptor.content[size] = 0;

  return descriptor;
}

char* listAsString(List* src) {
  if (src->element_size != sizeof(char))
    return nullptr;
  
  char* buffer = (char*) allocate(src->size);
  memcpy(buffer, src->elements, src->size);
  buffer[src->size] = 0;
  return buffer;
}

List listOf(size_t element_size) {
  List ret = {
    .capacity = LIST_CAPACITY_BATCH,
    .element_size = element_size,
    .size = 0
  };
  ret.elements = allocate(ret.capacity*ret.element_size);
  return ret;
}

int listPush(List* dst, void* element) {
  if (dst == nullptr) {
    perror("List pointer is null");
    return 1;
  }
  if (element == nullptr) {
    perror("Element pointer is null");
    return 1;
  }
  if (dst->size >= dst->capacity) {
    void* new_ptr = realloc(dst->elements, (dst->capacity + LIST_CAPACITY_BATCH)*(dst->element_size));
    if (new_ptr == nullptr) {
      perror("Cannot resize list");
      return 1;
    }
    dst->capacity += LIST_CAPACITY_BATCH;
    dst->elements = new_ptr;
  }
  if (memcpy(((byte*)dst->elements + ((dst->size++)*dst->element_size)), element, dst->element_size) == nullptr) {
    perror("Failed to copy memory");
    return 1;
  }
  return 0;
}

int listPop(List* src) {
  if (src == nullptr) {
    perror("List pointer is null");
    return 1;
  }
  if (src->size > 0) {
    memset((byte*) src->elements + (--src->size)*src->element_size, 0, src->element_size);
    double temp = (double)src->size / LIST_CAPACITY_BATCH;
    long neededCapacity = CCFG_ceil(temp)*LIST_CAPACITY_BATCH;
    if (neededCapacity <= 0)
      neededCapacity = LIST_CAPACITY_BATCH;
    if (src->capacity != neededCapacity) {
      void* new_ptr = realloc(src->elements, neededCapacity*src->element_size);
      if (new_ptr == nullptr) {
        perror("Cannot resize list");
        return 1;
      }
      src->capacity = neededCapacity;
      src->elements = new_ptr;
    }
  }
}

bool _fileHasPeek(FileDescriptor *file, int amount) {
  return (file->index+amount >= 0 && file->index+amount < file->size);
}

char _filePeek(FileDescriptor* file, int amount) {
  return _fileHasPeek(file, amount) ? file->content[file->index+amount] : 0;
}

char fileConsume(FileDescriptor* file) {
  return fileHasPeek(file) ? file->content[file->index++] : 0;
}

bool fileTryConsume(FileDescriptor* file, char c) {
  if (filePeek(file) == c) {
    fileConsume(file);
    return true;
  }
  return false;
}

void error(const char* error, int line) {
  printf("An error has occurred. %s at line %d", error, line);
  exit(1);
}

void tokenize(List* output, FileDescriptor* file) {
  bool comment = false;
  bool multiline_comment = false;
  int line = 1;
  while (fileHasPeek(file)) {
    if (fileTryConsume(file, COMMENT)) {
      comment = true;
    } else if (fileTryConsume(file, MULTILINE_COMMENT)) {
      multiline_comment = !multiline_comment;
    } else if (fileTryConsume(file, '\n')) {
      line++;
      comment = false;
    } else if (comment || multiline_comment || filePeek(file) == ' ' || filePeek(file) == 0 || filePeek(file) == '\r') {
      fileConsume(file);
    } else if (fileTryConsume(file, ':')) {
      if (fileTryConsume(file, '=')) {
        Token temp = {
          .line = line,
          .type = CCFG_TK_ASSIGN,
          .value = nullptr
        };
        listPush(output, &temp);
      } else {
        error("Unknown Token", line);
      }
    } else if (filePeek(file) == '.' && !isdigit(_filePeek(file, 1))) {
      Token temp = {
        .line = line,
        .type = CCFG_TK_DOT,
        .value = nullptr
      };
      fileConsume(file);
      listPush(output, &temp);
    } else if (fileTryConsume(file, '[')) {
      Token temp = {
        .line = line,
        .type = CCFG_TK_OPEN_SQUARE,
        .value = nullptr
      };
      listPush(output, &temp);
    } else if (fileTryConsume(file, ']')) {
      Token temp = {
        .line = line,
        .type = CCFG_TK_CLOSE_SQUARE,
        .value = nullptr
      };
      listPush(output, &temp);
    } else if (fileTryConsume(file, '{')) {
      Token temp = {
        .line = line,
        .type = CCFG_TK_OPEN_CURLY,
        .value = nullptr
      };
      listPush(output, &temp);
    } else if (fileTryConsume(file, '}')) {
      Token temp = {
        .line = line,
        .type = CCFG_TK_CLOSE_CURLY,
        .value = nullptr
      };
      listPush(output, &temp);
    } else if (fileTryConsume(file, ',')) {
      Token temp = {
        .line = line,
        .type = CCFG_TK_COMMA,
        .value = nullptr
      };
      listPush(output, &temp);
    } else if (fileTryConsume(file, '\'')) {
      List buffer = listOf(sizeof(char));
      char tick = '\'';
      listPush(&buffer, &tick);
      bool found = false;
      while (fileHasPeek(file)) {
        if (fileTryConsume(file, '\'')) {
          listPush(&buffer, &tick);
          found = true;
          break;
        }
        char c = fileConsume(file);
        listPush(&buffer, &c);
      }
      if (!found)
        error("String literal not closed", line);
      Token temp = {
        .line = line,
        .type = CCFG_TK_LITERAL,
        .value = listAsString(&buffer)
      };
      listPush(output, &temp);
    } else {
      if (isalpha(filePeek(file))) {
        List buffer = listOf(sizeof(char));
        while (isalnum(filePeek(file)) || filePeek(file) == '_' || filePeek(file) == '-') {
          char c = fileConsume(file);
          listPush(&buffer, &c);
        }
        char* ident = listAsString(&buffer);
        Token temp;
        if (strcmp(ident, "null") == 0) {
          temp.type = CCFG_TK_LITERAL;
        } else if (strcmp(ident, "true") == 0) {
          temp.type = CCFG_TK_LITERAL;
        } else if (strcmp(ident, "false") == 0) {
          temp.type = CCFG_TK_LITERAL;
        } else {
          temp.type = CCFG_TK_IDENTIFIER;
        }
        temp.line = line;
        temp.value = ident;
        listPush(output, &temp);
      } else if (isdigit(filePeek(file))) {
        List buffer = listOf(sizeof(char));
        while (isdigit(filePeek(file)) || filePeek(file) == '.') {
          char c = fileConsume(file);
          listPush(&buffer, &c);
        }
        Token temp = {
          .line = line,
          .type = CCFG_TK_LITERAL,
          .value = listAsString(&buffer)
        };
        listPush(output, &temp);
      } else if (filePeek(file) == '.' && isdigit(_filePeek(file, 1))) {
        List buffer = listOf(sizeof(char));
        char c = '0';
        listPush(&buffer, &c);
        c = '.';
        listPush(&buffer, &c);
        fileConsume(file);
        while (isdigit(filePeek(file))) {
          c = fileConsume(file);
          listPush(&buffer, &c);
        }
        Token temp = {
          .line = line,
          .type = CCFG_TK_LITERAL,
          .value = listAsString(&buffer)
        };
        listPush(output, &temp);
      } else {
        error("Unknown Token", line);
      }
    }
  }
}

void printfToken(Token* token) {
  switch (token->type) {
    case CCFG_TK_ASSIGN :
      printf(":=");
      break;
    case CCFG_TK_CLOSE_SQUARE :
      printf("]");
      break;
    case CCFG_TK_DOT :
      printf(".");
      break;
    case CCFG_TK_IDENTIFIER :
      printf("IDENT");
      break;
    case CCFG_TK_LITERAL :
      printf("LITERAL");
      break;
    case CCFG_TK_OPEN_SQUARE :
      printf("[");
      break;
    case CCFG_TK_COMMA :
      printf(", ");
      break;
    case CCFG_TK_OPEN_CURLY :
      printf("{");
      break;
    case CCFG_TK_CLOSE_CURLY :
      printf("}");
      break;
    default:
      printf("NULL");
      break;
  }
  if (token->value != nullptr) {
    printf("(%s);", token->value);
  }
}

void printfTokens(List* tokens) {
  for (int i = 0; i < tokens->size; i++) {
    Token* token = &cast(Token, tokens->elements)[i];
    printfToken(token);
    printf(" ");
  }
}

TokenStream createTokenStream(List *tokens) {
  TokenStream stream;
  stream.content = (Token*) allocate(tokens->size*sizeof(Token));
  stream.size = tokens->size;
  memcpy(stream.content, tokens->elements, tokens->size*tokens->element_size);
  stream.index = 0;
  return stream;
}

bool _tokenStreamHasPeek(TokenStream *stream, int amount) {
  return (stream->index+amount >= 0 && stream->index+amount < stream->size);
}

Token _tokenStreamPeek(TokenStream* stream, int amount) {
  return _tokenStreamHasPeek(stream, amount) ? stream->content[stream->index+amount] : EMPTY_TOKEN;
}

Token tokenStreamConsume(TokenStream* stream) {
  return tokenStreamHasPeek(stream) ? stream->content[stream->index++] : EMPTY_TOKEN;
}

bool _tokenStreamPeekEqual(TokenStream* stream, CCFG_TokenType t, char* val) {
  Token c = tokenStreamPeek(stream);
  bool valuesEqual = ((val == nullptr || c.value == nullptr) || (strcmp(val, c.value) == 0));
  return t == c.type && valuesEqual;
}

bool _tokenStreamTryConsume(TokenStream* stream, CCFG_TokenType t, char* val) {
  if (tokenStreamPeekEqual(stream, t)) {
    tokenStreamConsume(stream);
    return true;
  }
  return false;
}

Token _tokenStreamTryConsumeError(TokenStream* stream, CCFG_TokenType t, char* val, const char* err) {
  if (tokenStreamPeekEqual(stream, t)) {
    return tokenStreamConsume(stream);
  }
  error(err, _tokenStreamPeek(stream, -1).line);
}

CCFG_Table* newTable() {
  CCFG_Table* ret = (CCFG_Table*)malloc(sizeof(CCFG_Table));
  ret->capacity = LIST_CAPACITY_BATCH;
  ret->size = 0;
  ret->keys = allocate(ret->capacity*sizeof(char*));
  ret->values = allocate(ret->capacity*sizeof(CCFG_Object));
  return ret;
}

size_t tableContains(CCFG_Table* tbl, char* name) {
  for (int i = 0; i < tbl->size; i++) {
    if (strcmp(tbl->keys[i], name) == 0) {
      return i;
    }
  }
  return -1;
}

int addToTable(CCFG_Table* tbl, char* name, CCFG_Object* value) {
  if (tbl == nullptr) {
    perror("Table pointer is null");
    return 1;
  }
  if (name == nullptr) {
    perror("Key name is null");
    return 1;
  }
  int index;
  if ((index = tableContains(tbl, name)) > -1) {
    tbl->values[index] = *value;
    return 0;
  }

  if (tbl->size >= tbl->capacity) {
    void* new_ptr = realloc(tbl->keys, (tbl->capacity + LIST_CAPACITY_BATCH)*sizeof(char*));
    if (new_ptr == nullptr) {
      perror("Cannot resize list");
      return 1;
    }
    tbl->keys = new_ptr;
    void* new_ptr2 = realloc(tbl->values, (tbl->capacity + LIST_CAPACITY_BATCH)*sizeof(CCFG_Object));
    if (new_ptr2 == nullptr) {
      perror("Cannot resize list");
      tbl->keys = realloc(tbl->keys, tbl->capacity * sizeof(char*));
      return 1;
    }
    tbl->values = new_ptr2;
    tbl->capacity += LIST_CAPACITY_BATCH;
  }

  tbl->keys[tbl->size] = name;
  tbl->values[tbl->size] = *value;
  tbl->size++;

  return 0;

}

bool isNumber(char* s) {
  char* temp = strdup(s);
  while (*temp != 0) {
    if (!isdigit(*temp)) {
      return false;
    }
    temp++;
  }
  return true;
}

bool isFloat(char* s) {
  char* temp = strdup(s);
  bool hasDot = false;
  while (*temp != 0) {
    if (!isdigit(*temp)) {
      if (*temp == '.') {
        hasDot = true;
        temp++;
        continue;
      }
      return false;
    }
    temp++;
  }
  return hasDot;
}

CCFG_Object* parseLiteral(char* lit) {
  CCFG_Object* obj = (CCFG_Object*)malloc(sizeof(CCFG_Object));
  if (strcmp(lit, "null") == 0) {
    obj->type = CCFG_NULL;
  } else if (strcmp(lit, "true") == 0) {
    obj->type = CCFG_BOOLEAN;
    obj->value.b = true;
  } else if (strcmp(lit, "false") == 0) {
    obj->type = CCFG_BOOLEAN;
    obj->value.b = false;
  } else if (isNumber(lit)) {
    obj->type = CCFG_INTEGER;
    obj->value.i = atoll(lit);
  } else if (isFloat(lit)) {
    obj->type = CCFG_DOUBLE;
    obj->value.d = atof(lit);
  } else if (lit[0] == '\'') {
    size_t size = strlen(lit)-1;
    obj->type = CCFG_STRING;
    obj->value.s = (char*)malloc(size);
    memcpy(obj->value.s, &(lit[1]), size-1);
    obj->value.s[size-1] = 0;
  }
  return obj;
}

CCFG_Object* parseValue(TokenStream* stream) {
  if (tokenStreamPeekEqual(stream, CCFG_TK_LITERAL)) {
    char* lit = strdup(tokenStreamConsume(stream).value);
    return parseLiteral(lit);
  } else if (tokenStreamTryConsume(stream, CCFG_TK_OPEN_SQUARE)) {
    CCFG_Object* ret = (CCFG_Object*)malloc(sizeof(CCFG_Object));
    ret->type = CCFG_ARRAY;
    ret->value.arr = (List*)malloc(sizeof(List));
    *(ret->value.arr) = listOf(sizeof(CCFG_Object));
    bool found = false;
    int value_amount = 0;
    while (tokenStreamHasPeek(stream)) {
      if (tokenStreamTryConsume(stream, CCFG_TK_CLOSE_SQUARE)) {
        found = true;
        break;
      }
      if (value_amount > 0)
        tokenStreamTryConsumeError(stream, CCFG_TK_COMMA, "Expected ','");
      CCFG_Object* value = parseValue(stream);
      listPush(ret->value.arr, value);
      value_amount++;
    }
    return ret;
  } else
    error("Unknown Value", _tokenStreamPeek(stream, -1).line);
}

void parseObject(TokenStream* stream, CCFG_Table* parent) {
  if (tokenStreamTryConsume(stream, CCFG_TK_DOT)) {
    char* tbl_name = strdup(tokenStreamTryConsumeError(stream, CCFG_TK_IDENTIFIER, "Expected identifier").value);
    CCFG_Object* obj = (CCFG_Object*)malloc(sizeof(CCFG_Object));
    obj->type = CCFG_TABLE;
    obj->value.tbl = newTable();
    tokenStreamTryConsumeError(stream, CCFG_TK_OPEN_CURLY, "Expected '{'");
    bool found = false;
    while (tokenStreamHasPeek(stream)) {
      if (tokenStreamTryConsume(stream, CCFG_TK_CLOSE_CURLY)) {
        found = true;
        break;
      }
      parseObject(stream, obj->value.tbl);
    }
    if (!found)
      error("Expected '}'", _tokenStreamPeek(stream, -1).line);

    addToTable(parent, tbl_name, obj);
  } else if (tokenStreamPeekEqual(stream, CCFG_TK_IDENTIFIER)) {
    char* identifier = strdup(tokenStreamConsume(stream).value);
    tokenStreamTryConsumeError(stream, CCFG_TK_ASSIGN, "Expected ':='");
    CCFG_Object* value = parseValue(stream);

    addToTable(parent, identifier, value);
  }
}

CCFG_Object* parse(TokenStream* stream) {
  CCFG_Object* obj = (CCFG_Object*)malloc(sizeof(CCFG_Object));
  obj->type = CCFG_TABLE;
  obj->value.tbl = newTable();

  while (tokenStreamHasPeek(stream)) {
    parseObject(stream, obj->value.tbl);
  }

  return obj;
}

void printMultiple(const char* s, long amount) {
  for (int i = 0; i < amount; i++)
    printf("%s", s);
}

void _printObject(CCFG_Object* obj, long depth) {
  switch (obj->type) {
    case CCFG_NULL :
      printf("null");
      break;
    case CCFG_INTEGER :
      printf("%lld", obj->value.i);
      break;
    case CCFG_DOUBLE :
      printf("%Lf", obj->value.d);
      break;
    case CCFG_STRING :
      printf("'%s'", obj->value.s);
      break;
    case CCFG_BOOLEAN :
      printf("%s", ((obj->value.b) ? "true" : "false"));
      break;
    case CCFG_ARRAY :
      printf("[");
      List* lst = obj->value.arr;
      for (int i = 0; i < lst->size; i++) {
        if (i > 0)
          printf(", ");
        _printObject(&(cast(CCFG_Object, lst->elements)[i]), depth);
      }
      printf("]");
      break;
    case CCFG_TABLE :
      printf("{\n");
      CCFG_Table* tbl = obj->value.tbl;
      for (int i = 0; i < tbl->size; i++) {
        printMultiple("\t", depth);
        printf("%s : ", tbl->keys[i]);
        _printObject(&(tbl->values[i]), depth+1);
        printf(", \n");
      }
      printMultiple("\t", depth);
      printf("}");
      break;
  }
}

CCFG_Object* parseFile(const char* fname) {
  FileDescriptor file = CCFG_fopen(fname);
  List tokens = listOf(sizeof(Token));
  tokenize(&tokens, &file);
  TokenStream stream = createTokenStream(&tokens);
  CCFG_Object* val = parse(&stream);
  return val;
}

CCFG_Primitive getPrimitive(CCFG_Object* obj) {
  char buf[255] = {0};
  switch (obj->type) {
    case CCFG_NULL :
      return (CCFG_Primitive) {
        .isNull = true, 
        .isPresent = true,
        .toBool = false,
        .toDouble = (double) 0,
        .toLong = 0,
        .toString = strdup(""),
      };
    case CCFG_INTEGER :
      sprintf(buf, "%lld", obj->value.i);
      return (CCFG_Primitive) {
        .isNull = false, 
        .isPresent = true,
        .toBool = obj->value.i != 0,
        .toDouble = (double) obj->value.i,
        .toLong = obj->value.i,
        .toString = strdup(buf),
      };
    case CCFG_DOUBLE :
      sprintf(buf, "%.17g", obj->value.d);
      return (CCFG_Primitive) {
        .isNull = false, 
        .isPresent = true,
        .toBool = (long) obj->value.d != 0,
        .toDouble = obj->value.d,
        .toLong = (long) obj->value.d,
        .toString = strdup(buf),
      };
    case CCFG_STRING :
      return (CCFG_Primitive) {
        .isNull = false, 
        .isPresent = true,
        .toBool = strlen(obj->value.s) > 0,
        .toDouble = atof(obj->value.s),
        .toLong = atoll(obj->value.s),
        .toString = strdup(obj->value.s),
      };
    case CCFG_BOOLEAN :
      return (CCFG_Primitive) {
        .isNull = false, 
        .isPresent = true,
        .toBool = obj->value.b,
        .toDouble = (double) obj->value.b,
        .toLong = (long) obj->value.b,
        .toString = strdup((obj->value.b) ? "true" : "false"),
      };
    default:
      return (CCFG_Primitive) {
        .isPresent = false
      };
  }
}

CCFG_Array* getArrayFromList(List* lst) {
  CCFG_Array* arr = (CCFG_Array*)malloc(sizeof(CCFG_Array));

  arr->size = lst->size;
  arr->array = (CCFG_Any*)malloc(arr->size*sizeof(CCFG_Any));

  for (int i = 0; i < lst->size; i++) {
    CCFG_Primitive p = getPrimitive(&cast(CCFG_Object, lst->elements)[i]);
    if (p.isPresent) {
      arr->array[i].composite.isPresent = false;
      arr->array[i].primitive = p;
    } else {
      CCFG_Composite c = getComposite(&cast(CCFG_Object, lst->elements)[i]);
      if (!c.isPresent) {
        arr->array[i].primitive.isPresent = false;
        arr->array[i].composite.isPresent = false;
        continue;
      }
      arr->array[i].primitive.isPresent = false;
      arr->array[i].composite = c;
    }
  }

  return arr;
}

long stfind(CCFG_SimpleTable* st, char* key) {
  for (int i = 0; i < st->size; i++) {
    if (strcmp(st->keys[i], key) == 0) {
      return i;
    }
  }
  return -1;
}

CCFG_Any stget(CCFG_SimpleTable* st, char* key) {
  long index = stfind(st, key);
  if (index < 0)
    return (CCFG_Any) {
      .composite.isPresent = false,
      .primitive.isPresent = false
    };
  return st->values[index];
}

CCFG_SimpleTable* getSTBLFromTable(CCFG_Table* tbl) {
  CCFG_SimpleTable* simpleTbl = (CCFG_SimpleTable*)malloc(sizeof(CCFG_SimpleTable));
  simpleTbl->size = tbl->size;
  simpleTbl->keys = (char**)malloc(simpleTbl->size*sizeof(char*));
  simpleTbl->values = (CCFG_Any*)malloc(simpleTbl->size*sizeof(CCFG_Any));
  for (int i = 0; i < simpleTbl->size; i++) {
    simpleTbl->keys[i] = strdup(tbl->keys[i]);
    CCFG_Primitive p = getPrimitive(&cast(CCFG_Object, tbl->values)[i]);
    if (p.isPresent) {
      simpleTbl->values[i].composite.isPresent = false;
      simpleTbl->values[i].primitive = p;
    } else {
      CCFG_Composite c = getComposite(&cast(CCFG_Object, tbl->values)[i]);
      if (!c.isPresent) {
        simpleTbl->values[i].primitive.isPresent = false;
        simpleTbl->values[i].composite.isPresent = false;
        continue;
      }
      simpleTbl->values[i].primitive.isPresent = false;
      simpleTbl->values[i].composite = c;
    }
  }
  return simpleTbl;
}

CCFG_Composite getComposite(CCFG_Object* obj) {
  if (obj->type != CCFG_TABLE && obj->type != CCFG_ARRAY) {
    return (CCFG_Composite) {
      .isPresent = false
    };
  }

  if (obj->type == CCFG_TABLE) {
    return (CCFG_Composite) {
      .isPresent = true,
      .toArray = nullptr,
      .toTable = getSTBLFromTable(obj->value.tbl)
    };
  }

  return (CCFG_Composite) {
    .isPresent = true,
    .toTable = nullptr,
    .toArray = getArrayFromList(obj->value.arr)
  };
}

CCFG_Any geti(CCFG_Object* obj, long i) {
  CCFG_Composite c = getComposite(obj);
  if (!c.isPresent || c.toArray == nullptr)
    return (CCFG_Any) {
      .composite.isPresent = false,
      .primitive.isPresent = false
    };
  if (i < 0 || i >= c.toArray->size)
    return (CCFG_Any) {
      .composite.isPresent = false,
      .primitive.isPresent = false
    };
  return c.toArray->array[i];
}

CCFG_Any getk(CCFG_Object* obj, char* key) {
  CCFG_Composite c = getComposite(obj);
  if (!c.isPresent || c.toTable == nullptr)
    return (CCFG_Any) {
      .composite.isPresent = false,
      .primitive.isPresent = false
    };

  long index = stfind(c.toTable, key);
  if (index < 0)
    return (CCFG_Any) {
      .composite.isPresent = false,
      .primitive.isPresent = false
    };
  return c.toTable->values[index];
}
