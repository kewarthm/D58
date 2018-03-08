struct field {
	// Each key only holds 2047 bytes + \0
	char key[2048];
	char value[2048]
	struct field *next = NULL;
};

struct HTTPResponseHeader {
	int statusCode = 500;
	struct field *headers;
	char *body;
}

int _headerSetField(struct HTTPResponseHeader, char *key, char *value);
int _headerGetField(struct HTTPResponseHeader);
int _headerRelease();

