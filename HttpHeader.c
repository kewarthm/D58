#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

#define HEADER_VALUE_LENGTH 2048
#define HEADER_KEY_RESERVE_BYTES 2
#define HEADER_VAL_RESERVE_BYTES 3
#define HEADER_BODY_TYPE_NONE   0
#define HEADER_BODY_TYPE_STR    1
#define HEADER_BODY_TYPE_FILE   2

struct field {
	// Each key only holds 2047 bytes + \0
	char key[HEADER_VALUE_LENGTH];
	char value[HEADER_VALUE_LENGTH];
	struct field *next;
};

struct HTTPResponseHeader {
	short statusCode;
	struct field *fields;
	short bodyType;
	void *body;
	struct field *(*setField)(struct HTTPResponseHeader *, const char *, char *);
	struct field *(*getField)(struct HTTPResponseHeader *, const char *);
    char *(*compile)(struct HTTPResponseHeader *);
    void (*release)(struct HTTPResponseHeader *, int);
};

void headerInit(struct HTTPResponseHeader *header);
struct field *_headerSetField(struct HTTPResponseHeader *header, const char *key, char *value);
struct field *_headerGetField(struct HTTPResponseHeader *header, const char *key);
char *_headerCompile(struct HTTPResponseHeader *header);
void _headerRelease(struct HTTPResponseHeader *header, int dynamic);


struct field * _headerSetField(struct HTTPResponseHeader *header, const char *key, char *value) {
    struct field *curr = header->fields;
    struct field **parentNext = &header->fields;
    size_t len_k = strlen(key);
    
    while(curr) {
        if(strncasecmp(curr->key, key, len_k) == 0) {
            strncpy(curr->value, value, HEADER_VALUE_LENGTH - HEADER_KEY_RESERVE_BYTES);
            return curr;
        }
        parentNext = &curr->next;
        curr = curr->next;
    }
    
    if(curr = malloc(sizeof(struct HTTPResponseHeader))) {
        strncpy(curr->key,   key,   HEADER_VALUE_LENGTH - HEADER_KEY_RESERVE_BYTES);
        strncpy(curr->value, value, HEADER_VALUE_LENGTH - HEADER_VAL_RESERVE_BYTES);
        curr->next = NULL;
        *parentNext = curr;
        return curr;
    }
    return NULL;
}

struct field *_headerGetField(struct HTTPResponseHeader *header, const char *key) {
    struct field *curr = header->fields;
    while(curr) {
        if(strcasecmp(curr->key, key) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

char *_headerCompile(struct HTTPResponseHeader *header) {
    static struct HTTPResponseHeader *_prevHeader = NULL;
    static struct field *_prevField = NULL;
    static int _statusCompiled = 0;
    static int _bodyCompiled = 0;
    if(header && header != _prevHeader) {
        _prevHeader = header;
        _prevField = header->fields;
        _statusCompiled = 0;
        _bodyCompiled = 0;
    }
    
    if(_prevHeader) {
        char buffer[HEADER_VALUE_LENGTH * 2] = {0};
        if(!_statusCompiled) {
            
        } else if(_prevField) {
            struct field *curr = _prevField;
            strcat(buffer, curr->key);
            strcat(buffer, ": ");
            strcat(buffer, curr->value);
            strcat(buffer, "\r\n");
            _prevField = curr->next;
            return strdup(buffer);
        } else if(header->bodyType && !_bodyCompiled) {
            char *rslt = NULL;
            switch(header->bodyType) {
                case HEADER_BODY_TYPE_STR:
                    rslt = (char *)header->body;
                    break;
                case HEADER_BODY_TYPE_FILE: {
                        size_t count = 0;
                        size_t sizeToRead = HEADER_VALUE_LENGTH * 2 - 1;
                        while(count = read(*((int *)header->body), buffer + count, sizeToRead) > 0) {
                            sizeToRead -= count;
                        }
                        rslt = strdup(buffer);
                    }
                    break;
                default:
                    break;
            }
            _bodyCompiled = !0;
            return rslt;
        }
    }
    return NULL;
}

void _headerRelease(struct HTTPResponseHeader *header, int dynamic) {
    struct field *curr = header->fields;
    while(header->fields) {
        curr = header->fields->next;
        header->fields = curr->next;
        free(curr);
    }
    
    if(dynamic) free(header);
}

void headerInit(struct HTTPResponseHeader *header) {
    header->setField = _headerSetField;
    header->getField = _headerGetField;
    header->compile = _headerCompile;
    header->release = _headerRelease;
    header->statusCode = 500;
    header->fields = NULL;
    header->bodyType = HEADER_BODY_TYPE_NONE;
    header->body = NULL;
}

int main() {
    struct HTTPResponseHeader testHeader = {0};
    struct field f0 = {"Hello", "World", 0};
    struct field f1 = {"Header1", "Value1", 0};
    struct field f2 = {"Last-Modified", "sometime", 0};
    struct field f3 = {"content-type", "text/plain", 0};
    char *someData = "Super long and tedious data\n";
    headerInit(&testHeader);
    testHeader.fields = &f0;
    f0.next = &f1;
    f1.next = &f2;
    f2.next = &f3;
    testHeader.body = someData;
    testHeader.bodyType = HEADER_BODY_TYPE_STR;
    
    char *rslt = NULL;
    printf("Compiled header:\n");
    while(rslt = testHeader.compile(&testHeader)) {
        printf("%s", rslt);
    }
    
    
}
