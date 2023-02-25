#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <stdint.h>

#include <camlib.h>

extern struct PtpRuntime ptp_runtime;

int ptp_canon_activate_command(struct PtpRuntime *r);
int ptp_canon_exec_evproc(struct PtpRuntime *r, void *data, int length);

// TODO:
//  Parse hex into int
//  Parse filenames
//  Seperate parser and packer
//  don't hardcode parser lengths

// Structs are sent in little endian
struct EvProcFooter {
	uint32_t params; // Number of parameters
	uint32_t longpars; // Number of long parameters (string, file)
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

int alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int digit(char c)
{
	return (c >= '0' && c <= '9');
}

int hex(char c)
{
	return digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

// Parse a formatted command into struct Tokens
// Should parse:
//  ThisCommand   123 "A String" 0xabc
struct Tokens parseCommand(char string[])
{
	struct Tokens toks;
	int t = 0;

	int c = 0;
	while (string[c] != '\0') {
		while (string[c] == ' ' || string[c] == '\t') {
			c++;
		}

		int s = 0;
		if (alpha(string[c])) {
			toks.t[t].type = TOK_TEXT;
			while (alpha(string[c])) {
				toks.t[t].string[s] = string[c];
				c++;
				s++;

				if (s > MAX_STR) {
					break;
				}
			}
		} else if (string[c] == '0' && string[c + 1] == 'x') {
			toks.t[t].integer = 0;
			toks.t[t].type = TOK_INT;
			c += 2;
			while (hex(string[c])) {
				toks.t[t].integer *= 16;
				if (string[c] >= '0' && string[c] <= '9') {
					toks.t[t].integer += string[c] - '0';
				}

				if (string[c] >= 'A' && string[c] <= 'F') {
					toks.t[t].integer += string[c] - 'A' + 10;
				}

				if (string[c] >= 'a' && string[c] <= 'f') {
					toks.t[t].integer += string[c] - 'a' + 10;
				}

				c++;
			}
		} else if (digit(string[c])) {
			toks.t[t].integer = 0;
			toks.t[t].type = TOK_INT;
			while (digit(string[c])) {
				toks.t[t].integer *= 10;
				toks.t[t].integer += string[c] - '0';
				c++;
			}
		} else if (string[c] == '"') {
			toks.t[t].type = TOK_STR;
			c++;
			while (string[c] != '"') {
				toks.t[t].string[s] = string[c];
				c++;
				s++;

				if (s > MAX_STR) {
					break;
				}
			}
			c++;
		} else {
			printf("Skipping unknown character '%c'\n", string[c]);
			c++;
			continue;
		}

		toks.t[t].string[s] = '\0';

		if (t >= MAX_TOK) {
			printf("Hit max parameter count, killing parser.\n");
			break;
		} else {
			t++;
		}
	}

	toks.length = t;

	return toks;
}

// Will return PTP status code.
// Returns "1" on parse error.
int evproc_run(char string[])
{
	struct EvProcFooter footer;
	footer.params = 0;
	footer.longpars = 0;

	struct Tokens toks = parseCommand(string);

	// Simple bit of code the testing the parser
#if 0
	printf("Command AST for '%s':\n", string);
	for (int i = 0; i < toks.length; i++) {
		switch (toks.t[i].type) {
		case TOK_TEXT:
			printf("Found text token: %s\n", toks.t[i].string);
			break;
		case TOK_STR:
			printf("Found string token: \"%s\"\n", toks.t[i].string);
			break;
		case TOK_INT:
			printf("Found integer token: %d\n", toks.t[i].integer);
			break;			
		}
	}
	return 0;
#endif

	char data[1024];
	int curr = 0;

	if (toks.length == 0) {
		puts("Error, must have at least 1 parameter.");
		return 1;
	}

	// Add in initial parameter
	if (toks.t[0].type == TOK_TEXT) {
		int len = strlen(toks.t[0].string);
		memcpy(data, toks.t[0].string, len);
		data[len] = '\0';
		curr += len + 1;
	} else {
		puts("Error, first parameter must be plain text.");
		return 1;
	}

	// Pack parameters into data
	for (int t = 1; t < toks.length; t++) {
		switch (toks.t[t].type) {
		case TOK_INT: {
			struct EvProcInt integer;
			memset(&integer, 0, sizeof(struct EvProcInt));
			integer.number = toks.t[t].integer;

			memcpy(data + curr, &integer, sizeof(struct EvProcInt));
			curr += sizeof(struct EvProcInt);

			footer.params++;
		} break;
		case TOK_STR: {
			struct EvProcStr string;
			memset(&string, 0, sizeof(struct EvProcStr));

			string.type = 4;
			string.size = strlen(toks.t[t].string);

			memcpy(data + curr, &string, sizeof(struct EvProcStr));
			curr += sizeof(struct EvProcStr);

			memcpy(data + curr, toks.t[t].string, string.size + 1);
			curr += string.size + 1;

			footer.params++;
			footer.longpars++;
		} break;
		}
	}

	// Add in the evproc footer
	memcpy(data + curr, &footer, sizeof(struct EvProcFooter));
	curr += sizeof(struct EvProcFooter);

	// Command is disabled on some cams, run it nonetheless
	int ret = ptp_canon_activate_command(&ptp_runtime);
	if (ret) {
		printf("Error activating command %d\n", ret);
		ptp_device_close(&ptp_runtime);
		return 1;
	}

	ret = ptp_canon_exec_evproc(&ptp_runtime, data, curr);

	ptp_device_close(&ptp_runtime);
	return ret;
}
