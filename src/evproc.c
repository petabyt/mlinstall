// Evproc parser
// mlinstall (Apache License)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <camlib.h>

int ptp_canon_activate_command(struct PtpRuntime *r);
int ptp_canon_exec_evproc(struct PtpRuntime *r, void *data, int length);

// TODO:
//  Parse hex into int
//  Parse filenames
//  Seperate parser and packer
//  don't hardcode parser lengths

// Structs are sent in little endian
struct EvProcFooter {
	uint32_t long_args;
	struct EvProcFooterArgs {
		uint32_t long_arg_index;
		uint32_t long_arg_length;
	}args[];
};

struct EvProcInt {
	uint32_t type;
	uint32_t number;
	uint32_t p3;
	uint32_t p4;
	uint32_t size;
};

struct EvProcStr {
	uint32_t type;
	uint32_t number;
	uint32_t p3;
	uint32_t p4;
	uint32_t size;
	// Followed by NULL terminated string
};

enum Types {
	TOK_TEXT,
	TOK_STR,
	TOK_INT,
};

#define MAX_STR 128
#define MAX_TOK 10

struct Tokens {
	struct T {
		int type;
		char string[MAX_STR];
		int integer;
	} t[MAX_TOK];
	int length;
};

int alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int digit(char c) {
	return (c >= '0' && c <= '9');
}

int hex(char c) {
	return digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

// Parse a formatted command into struct Tokens
// Should parse:
//  ThisCommand   123 "A String" 0xabc
struct Tokens *tokenize_evproc_command(char string[]) {
	struct Tokens *toks = malloc(sizeof(struct Tokens));
	memset(toks, 0, sizeof(struct Tokens));
	int t = 0;

	int c = 0;
	while (string[c] != '\0') {
		// Skip whitespace chars
		while (string[c] == ' ' || string[c] == '\t') {
			c++;
		}

		if (alpha(string[c])) {
			int s = 0;
			toks->t[t].type = TOK_TEXT;
			while (alpha(string[c])) {
				toks->t[t].string[s] = string[c];
				c++;
				s++;

				if (s > MAX_STR) {
					break;
				}
			}
			toks->t[t].string[s] = '\0';
		} else if (string[c] == '0' && string[c + 1] == 'x') {
			// Crappy hex parser (don't needs more testing)
			toks->t[t].integer = 0;
			toks->t[t].type = TOK_INT;
			c += 2;
			while (hex(string[c])) {
				toks->t[t].integer *= 16;
				if (string[c] >= '0' && string[c] <= '9') {
					toks->t[t].integer += string[c] - '0';
				}

				if (string[c] >= 'A' && string[c] <= 'F') {
					toks->t[t].integer += string[c] - 'A' + 10;
				}

				if (string[c] >= 'a' && string[c] <= 'f') {
					toks->t[t].integer += string[c] - 'a' + 10;
				}

				c++;
			}
		} else if (digit(string[c])) {
			toks->t[t].integer = 0;
			toks->t[t].type = TOK_INT;
			while (digit(string[c])) {
				toks->t[t].integer *= 10;
				toks->t[t].integer += string[c] - '0';
				c++;
			}
		} else if (string[c] == '"') {
			c++;
			toks->t[t].type = TOK_STR;
			int s = 0;
			while (string[c] != '"') {
				toks->t[t].string[s] = string[c];
				c++;
				s++;

				if (s > MAX_STR) {
					break;
				}
			}
			c++;
			toks->t[t].string[s] = '\0';
		} else {
			printf("Skipping unknown character '%c'\n", string[c]);
			c++;
			continue;
		}

		if (t >= MAX_TOK) {
			printf("Error: Hit max parameter count.\n");
			return NULL;
		} else {
			t++;
		}
	}

	toks->length = t;

	return toks;
}

char *canon_evproc_pack(int *length, char *string) {
	struct EvProcFooter footer;
	footer.long_args = 0;

	struct Tokens *toks = tokenize_evproc_command(string);

	char *data = malloc(500);

	if (toks->length == 0) {
		puts("Error, must have at least 1 parameter.");
		return NULL;
	}

	// Add in initial parameter
	if (toks->t[0].type == TOK_TEXT) {
		int len = strlen(toks->t[0].string);
		memcpy(data, toks->t[0].string, len);
		data[len] = '\0';
		(*length) += len + 1;
	} else {
		puts("Error, first parameter must be plain text.");
		return NULL;
	}

	// This will be modified as the structure is built
	uint32_t *num_args = (uint32_t *)(data + (*length));
	(*num_args) = 0;
	(*length) += 4;

	// Pack parameters into data
	for (int t = 1; t < toks->length; t++) {
		switch (toks->t[t].type) {
		case TOK_INT: {
			struct EvProcInt integer;
			memset(&integer, 0, sizeof(struct EvProcInt));
			integer.type = 2;
			integer.number = toks->t[t].integer;

			memcpy(data + (*length), &integer, sizeof(struct EvProcInt));
			(*length) += sizeof(struct EvProcInt);

			(*num_args)++;
		} break;
		case TOK_STR: {
			struct EvProcStr string;
			memset(&string, 0, sizeof(struct EvProcStr));

			string.type = 4;
			string.size = strlen(toks->t[t].string);

			memcpy(data + (*length), &string, sizeof(struct EvProcStr));
			(*length) += sizeof(struct EvProcStr);

			memcpy(data + (*length), toks->t[t].string, string.size + 1);
			(*length) += string.size + 1;

			// TODO: Finish this
			footer.args[0].long_arg_index = (*num_args);
			footer.args[0].long_arg_length = string.size + 1234;
			footer.long_args++;
			(*num_args)++;
		} break;
		}
	}

	memcpy(data + (*length), &footer, sizeof(struct EvProcFooter));
	(*length) += sizeof(struct EvProcFooter);

	if (footer.long_args) {
		memcpy(data + (*length), &footer.args, sizeof(struct EvProcFooterArgs) * footer.long_args);
		(*length) += sizeof(struct EvProcFooterArgs) * footer.long_args;
	}

	free(toks);

	return data;	
}

// Will return PTP status code.
// Returns "1" on parse error.
int canon_evproc_run(struct PtpRuntime *r, char string[]) {
	// Command is disabled on some cams, run it nonetheless
	int ret = ptp_canon_activate_command(r);
	if (ret) {
		printf("Error activating command %d\n", ret);
		return ret;
	}

	int length = 0;
	void *data = canon_evproc_pack(&length, string);
	ret = ptp_canon_exec_evproc(r, data, length);
	free(data);

	return 123;
}
