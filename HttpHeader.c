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

void statusCodeToStr(int statusCode, char *outBuf) {
	switch(statusCode) {
		case 200:
			strcpy(outBuf, "200 OK");
			break;
		case 304:
			strcpy(outBuf, "304 Not Modified");
			break;
		case 400:
			strcpy(outBuf, "400 Bad Request");
			break;
		case 404:
			strcpy(outBuf, "404 Not Found");
			break;
		case 405:
			strcpy(outBuf, "405 Method Not Allowed");
			break;
		case 500:
			strcpy(outBuf, "500 Internal Server Error");
			break;
		case 505:
			strcpy(outBuf, "505 Version Not Supported");
			break;
		default:
			strcpy(outBuf, "500 Internal Server Error");
			break;
	}
}

struct field {
	// Each key only holds 2047 bytes + \0
	char key[HEADER_VALUE_LENGTH];
	char value[HEADER_VALUE_LENGTH];
	struct field *next;
};

struct HTTPResponseHeader {
	char version[16]; // Version string upto 15 characters + \0
	short statusCode;
	struct field *fields;
	short bodyType;
	void *body;
	void (*setVersion)(struct HTTPResponseHeader *, const char *);
	struct field *(*setField)(struct HTTPResponseHeader *, const char *, const char *);
	struct field *(*getField)(struct HTTPResponseHeader *, const char *);
	void (*setBody)(struct HTTPResponseHeader *, void *, short);
    char *(*compile)(struct HTTPResponseHeader *);
    void (*release)(struct HTTPResponseHeader *, int);
};

void headerInit(struct HTTPResponseHeader *header);
void _headerSetVersion(struct HTTPResponseHeader *header, const char *version);
struct field *_headerSetField(struct HTTPResponseHeader *header, const char *key, const char *value);
struct field *_headerGetField(struct HTTPResponseHeader *header, const char *key);
void _headerSetBody(struct HTTPResponseHeader *header, void *body, short bodyType);
char *_headerCompile(struct HTTPResponseHeader *header);
void _headerRelease(struct HTTPResponseHeader *header, int isDynamic);


void headerInit(struct HTTPResponseHeader *header) {
	header->setVersion = _headerSetVersion;
    header->setField = _headerSetField;
    header->getField = _headerGetField;
    header->setBody = _headerSetBody;
    header->compile = _headerCompile;
    header->release = _headerRelease;
    header->statusCode = 500;
    header->fields = NULL;
    header->bodyType = HEADER_BODY_TYPE_NONE;
    header->body = NULL;
}

void _headerSetVersion(struct HTTPResponseHeader *header, const char *version) {
	strncpy(header->version, version, 15);
}

struct field * _headerSetField(struct HTTPResponseHeader *header, const char *key, const char *value) {
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
    curr = malloc(sizeof(struct field));
    if(curr) {
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

void _headerSetBody(struct HTTPResponseHeader *header, void *data, short bodyType) {
	header->bodyType = bodyType;
	header->body = data;
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
			strcat(buffer, header->version);
			strcat(buffer, " ");
            statusCodeToStr(header->statusCode, buffer + strlen(header->version) + 1);
            strcat(buffer, "\r\n");
			_statusCompiled = !0;
			return strdup(buffer);
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

void _headerRelease(struct HTTPResponseHeader *header, int isDynamic) {
    struct field *curr = NULL;
    while(curr = header->fields) {
        header->fields = curr->next;
        free(curr);
    }
    
    if(isDynamic) free(header);
}

int main() {
    struct HTTPResponseHeader testHeader = {0};
    struct HTTPResponseHeader testHeader2 = {0};
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
	strcpy(testHeader.version, "HTTP/1.0");
    
    char *rslt = NULL;
    printf("Compiled header1:\n");
    while(rslt = testHeader.compile(&testHeader)) {
        printf("%s", rslt);
    }
	
    headerInit(&testHeader2);
	testHeader2.setVersion(&testHeader2, "HTTP/1.0");
	testHeader2.setField(&testHeader2, "Hello", "World");
	testHeader2.setField(&testHeader2, "Header1", "Value1");
	testHeader2.setField(&testHeader2, "Last-Modified", "sometime");
	testHeader2.setField(&testHeader2, "content-type", "text/plain");
	//testHeader2.setField(&testHeader2, "Header1", "Value1-New");
	testHeader2.setBody(&testHeader2, someData, HEADER_BODY_TYPE_STR);
	
	
    printf("Compiled header2:\n");
    while(rslt = testHeader.compile(&testHeader2)) {
        printf("%s", rslt);
    }
	
	//testHeader.release(&testHeader, 0);
	testHeader2.release(&testHeader2, 0);
	
}
