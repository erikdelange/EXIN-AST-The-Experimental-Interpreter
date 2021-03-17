/* scanner.c
 *
 * Token scanner (in literature also called symbol scanner)
 *
 * A program consist of a sequence of tokens. A token is a group of one or
 * more characters which have a special meaning in the programming language.
 * The scanner reads a program character by character and converts these
 * into tokens.
 *
 * Object 'scanner' is the API to the token scanner. Only one scanner object
 * exists. For its definition see scanner.h.
 *
 * The next token is read by calling 'scanner.next'. On return variable
 * 'scanner.token' contains the token and 'scanner.string' - if applicable - the
 * identifier, the number, the character or the string. In all other cases it
 * contains an empty string ("").
 *
 * Copyright (c) 1994 K.W.E. de Lange
 */
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "identifier.h"
#include "scanner.h"
#include "error.h"


/* Table containing all language keywords and their corresponding tokens.
 */
static struct {
	char *keyword;
	token_t token;
} keywordTable[] = {  /* Note: keyword strings must be sorted alphabetically */
	{ "and",		AND },
	{ "break",		BREAK },
	{ "char",		DEFCHAR },
	{ "continue",	CONTINUE },
	{ "def",		DEFFUNC },
	{ "do",			DO },
	{ "else",		ELSE },
	{ "float",		DEFFLOAT },
	{ "for",		FOR },
	{ "if",			IF },
	{ "import",		IMPORT },
	{ "in",			IN },
	{ "input",		INPUT },
	{ "int",		DEFINT },
	{ "list",		DEFLIST},
	{ "or",			OR },
	{ "pass",		PASS },
	{ "print",		PRINT },
	{ "return",		RETURN },
	{ "str",		DEFSTR },
	{ "while",		WHILE }
};


/* Minimal forward declarations.
 */
static token_t read_next_token(char *buffer, int bufsize);


/* Read the next character.
 *
 * return	the character read
 */
static int nextch(void)
{
	assert(scanner.module != NULL);

	return scanner.module->nextch(scanner.module);
}


/* Look ahead to see what the next character is, but don't read it.
 *
 * return	the next character to read
 */
static int peekch(void)
{
	assert(scanner.module != NULL);

	return scanner.module->peekch(scanner.module);
}


/* Undo the read of a character.
 *
 * ch		the character to push back into the input stream
 * return	the character which was pushed back
 */
static int pushch(const int ch)
{
	assert(scanner.module != NULL);

	return scanner.module->pushch(scanner.module, ch);
}


/* API: Initialize scanner object 'sc'.
 */
static void scanner_init(struct scanner *sc, Module *m)
{
	assert(sc != NULL);

	/* load function addresses from the global scanner
	 */
	*sc = scanner;

	/* reset all object variables to their initial state
	 */
	sc->module = NULL,

	sc->token = UNKNOWN,
	sc->peeked = 0,
	sc->at_bol = true,
	sc->string[0] = 0,

	sc->indentlevel = 0,
	sc->indentation[0] = 0,

	sc->module = m;  /* reading from this module */
}


/* API: Save the global scanner state in sc.
 */
static void scanner_save(struct scanner *sc)
{
	assert(sc != NULL);

	*sc = scanner;
}


/* API: Load the global scanner state from sc.
 */
static void scanner_load(struct scanner *sc)
{
	assert(sc != NULL);

	scanner = *sc;
}


/* API: Read the next token.
 *
 * return	token read
 *
 * If previously a peek was done then return the peeked token.
 */
static token_t next_token(void)
{
	if (scanner.peeked == 0)
		scanner.token = read_next_token(scanner.string, BUFSIZE);
	else {
		scanner.token = scanner.peeked;
		scanner.peeked = 0;
	}

	debug_printf(DEBUGTOKEN, "\ntoken : %s %s", \
							  tokenName(scanner.token), scanner.string);

	return scanner.token;
}


/* API: Look at the next token, without actually considering it read.
 *
 * return	peeked token
 *
 * Only a single peek is possible, you cannot look more then 1 token ahead.
 * Repeated peeks will all return the first value peeked.
 */
static token_t peek_token(void)
{
	if (scanner.peeked == 0)
		scanner.peeked = read_next_token(scanner.string, BUFSIZE);

	return scanner.peeked;
}


/* Read a string.
 *
 * string	pointer to a buffer where the string will be stored
 * bufsize	size of buffer including space for closing '\0'
 * return	token which was read (by definition STR)
 *
 * Strings are surrounded by double quotes. Escape sequences are recognized.
 *
 * Examples: "abc"  "xyz\n"  ""
 */
static token_t read_string(char *string, int bufsize)
{
	char ch;
	int count = 0;

	while (1) {
		ch = nextch();
		if (ch != EOF && ch != '\"') {
			if (ch == '\\')
				switch (peekch()) {
					case '0' :	nextch(); ch = '\0'; break;
					case 'a' :	nextch(); ch = '\a'; break;
					case 'b' :	nextch(); ch = '\b'; break;
					case 'f' :	nextch(); ch = '\f'; break;
					case 'n' :	nextch(); ch = '\n'; break;
					case 'r' :	nextch(); ch = '\r'; break;
					case 't' :	nextch(); ch = '\t'; break;
					case 'v' :	nextch(); ch = '\v'; break;
					case '\\':	nextch(); ch = '\\'; break;
					case '\'':	nextch(); ch = '\''; break;
					case '\"':	nextch(); ch = '\"'; break;
				}
			if (count < bufsize)
				string[count++]= ch;
		} else {
			string[count] = 0;
			break;
		}
	}
	return STR;
}


/* Read an integer or a floating point number.
 *
 * number	pointer to buffer with string representation of the number read
 * bufsize	size of buffer including space for closing '\0'
 * return	token which was read (INT or FLOAT)
 *
 * Scientific notation (e, E) is recognized.
 *
 * Examples: 2  2.  0.2  2.0  1E+2  1E2  1E-2  0.1e+2
 */
static token_t read_number(char *number, int bufsize)
{
	char ch;
	int dot = 0;
	int exp = 0;
	int count = 0;

	while (1) {
		ch = nextch();
		if (ch != EOF && (isdigit(ch) || ch == '.')) {
			if (ch == '.') {
				if (++dot > 1)
					raise(ValueError, "multiple decimal points");
			}
			if (count < bufsize)
				number[count++] = ch;
		} else {  /* check for scientific notation */
			if (ch == 'e' || ch == 'E') {
				exp = 1;
				if (count < bufsize)
					number[count++] = ch;
				ch = nextch();

				if (ch == '-' || ch == '+') {
					if (count < bufsize)
						number[count++] = ch;
					ch = nextch();
				}
				if (!isdigit(ch))
					raise(ValueError, "missing exponent");
				while (ch != EOF && isdigit(ch)) {
					if (count < bufsize)
						number[count++] = ch;
					ch = nextch();
				}
			}
			number[count] = 0;
			pushch(ch);
			break;
		}
	}

	if (dot == 1 || exp == 1)
		return FLOAT;

	return INT;
}


/* Read a name and check whether it is a keyword or an identifier.
 *
 * name		pointer to buffer with keyword or identifier
 * bufsize	size of buffer including space for closing '\0'
 * return	keyword token or IDENTIFIER in case of an identifier
 *
 * A name consist of digits, letters and underscores, and must
 * start with a letter.
 */
static token_t read_identifier(char *name, int bufsize)
{
	char ch;
	int count = 0, l, h, m = 0, d = 0;

	while (1) {
		ch = nextch();
		if (ch != EOF && (isalnum(ch) || ch == '_')) {
			if (count < bufsize)
				name[count++] = ch;
		} else {
			name[count] = 0;
			pushch(ch);
			break;
		}
	}

	l = 0, h = (int)(sizeof keywordTable / sizeof keywordTable[0]) - 1;

	while (l <= h) {
		m = (l + h) / 2;
		d = strcmp(&name[0], keywordTable[m].keyword);
		if (d < 0)
			h = m - 1;
		if (d > 0)
			l = m + 1;
		if (d == 0)
			break;
	};

	if (d == 0) {
		name[0] = 0;
		return keywordTable[m].token;
	} else
		return IDENTIFIER;
}


/* Read a character constant. This can be a single letter or an escape sequence.
 *
 * c		pointer to buffer with the character read
 * bufsize	size of buffer including space for closing '\0'
 * return	token which was read (by definition CHAR)
 *
 * A character constant is surrounded by single quotes.
 *
 * Examples: 'a'  '\n'
 */
static token_t read_character(char *c, int bufsize)
{
	UNUSED(bufsize);  /* bufsize unused, size of c[] assumed to be at least 2 */

	char ch;

	ch = nextch();

	if (ch == '\\') {  /* is an escape sequence */
		ch = nextch();
		switch (ch) {
			case '0' :	c[0] = '\0'; break;
			case 'a' :	c[0] = '\a'; break;
			case 'b' :	c[0] = '\b'; break;
			case 'f' :	c[0] = '\f'; break;
			case 'n' :	c[0] = '\n'; break;
			case 'r' :	c[0] = '\r'; break;
			case 't' :	c[0] = '\t'; break;
			case 'v' :	c[0] = '\v'; break;
			case '\\':	c[0] = '\\'; break;
			case '\'':	c[0] = '\''; break;
			case '\"':	c[0] = '\"'; break;
			default  :	raise(SyntaxError, "unknown escape sequence: %c", ch);
		}
	} else {  /* not an escape sequence */
		if (ch == '\'' || ch == EOF)
			raise(SyntaxError, "empty character constant");
		else
			c[0] = ch;
	}
	ch = nextch();
	if (ch != '\'')
		raise(SyntaxError, "to many characters in character constant");

	c[1] = 0;

	return CHAR;
}


/* Read the next token.
 *
 * buffer	pointer to buffer containing the token which was read
 * bufsize	size of buffer including space for closing '\0'
 * return	objecttype which was read
 *
 * After reading 'buffer' contains:
 *    the identifier if token == IDENTIFIER
 *    the number if token == INTEGER or FLOAT
 *    the string if token == STRING
 *    the character if token == CHAR
 *    and an empty string ("") for all other tokens
 */
static token_t read_next_token(char *buffer, int bufsize)
{
	char ch;

	assert(buffer != NULL);
	assert(bufsize >= 2);  /* at least space for 1 character plus closing '\0' */

	buffer[0] = 0;

	/* Determine the level of indentation. If it has increased compared to the
	 * previous line then token is INDENT. Has it decreased then check if it
	 * was equal to the previous (smaller) indentation. If so then the token
	 * is DEDENT, else there is an indentation error.
	 * If the indentation has not changed then continue reading the next token.
	 */
	while (scanner.at_bol == true) {
		int col = 0;
		scanner.at_bol = false;

		/* determine the indentation */
		while (1) {
			ch = nextch();
			if (ch == ' ')
				col++;
			else if (ch == '\t')
				col = (col / config.tabsize + 1) * config.tabsize;
			else
				break;
		}  /* col = column-nr of first character which is not tab or space */

		/* ignore empty lines or comment only lines */
		if (ch == '#')
			while (ch != '\n' && ch != EOF)
				ch = nextch();
		if (ch == '\r')
			ch = nextch();  /* handle linux style line ending */
		if (ch == '\n') {
			scanner.at_bol = true;
			continue;
		} else if (ch == EOF) {
			col = 0;  /* do we need more DEDENTs? */
			if (col == scanner.indentation[scanner.indentlevel])
				return ENDMARKER;
		} else
			pushch(ch);

		if (col == scanner.indentation[scanner.indentlevel])
			break;  /* indentation has not changed */
		else if (col > scanner.indentation[scanner.indentlevel]) {
			if (scanner.indentlevel == MAXINDENT)
				raise(SyntaxError, "max indentation level reached");
			scanner.indentation[++scanner.indentlevel] = col;
			return INDENT;
		} else {  /* col < scanner.indentation[scanner.level] */
			if (--scanner.indentlevel < 0)
				raise(SyntaxError, "inconsistent use of TAB and space in indentation");
			if (col != scanner.indentation[scanner.indentlevel]) {
				scanner.at_bol = true;  /* not yet at old indentation level */
				scanner.module->pos = scanner.module->bol;
			}
			return DEDENT;
		}
	}

	/* skip spaces */
	do {
		ch = nextch();
	} while (ch == ' ' || ch == '\t');

	/* skip comments */
	if (ch == '#')
		while (ch != '\n' && ch != EOF)
			ch = nextch();

	/* check for end of line or end of file */
	if (ch == '\r')
		ch = nextch();  /* handle linux style line endings */
	if (ch == '\n') {
		scanner.at_bol = true;
		return NEWLINE;
	} else if (ch == EOF)
		return ENDMARKER;

	if (isdigit(ch)) {
		pushch(ch);
		return read_number(buffer, bufsize);
	} else if (isalpha(ch)) {
		pushch(ch);
		return read_identifier(buffer, bufsize);
	} else {
		switch (ch) {
			case '\'':	return read_character(buffer, bufsize);
			case '\"': 	return read_string(buffer, bufsize);
			case EOF :	return ENDMARKER;
			case '(' :	return LPAR;
			case ')' :	return RPAR;
			case '[' :	return LSQB;
			case ']' :	return RSQB;
			case ',' :	return COMMA;
			case '.' :	return DOT;
			case ':' :	return COLON;
			case '*' :	if (peekch() == '=') {
							nextch();
							return STAREQUAL;
						} else
							return STAR;
			case '%' :	if (peekch() == '=') {
							nextch();
							return PERCENTEQUAL;
						} else
							return PERCENT;
			case '+' :	if (peekch() == '=') {
							nextch();
							return PLUSEQUAL;
						} else
							return PLUS;
			case '-' :	if (peekch() == '=') {
							nextch();
							return MINUSEQUAL;
						} else
							return MINUS;
			case '/' :	if (peekch() == '=') {
							nextch();
							return SLASHEQUAL;
						} else
							return SLASH;
			case '!' :	if (peekch() == '=') {
							nextch();
							return NOTEQUAL;
						} else
							return NOT;
			case '=' :	if (peekch() == '=') {
							nextch();
							return EQEQUAL;
						} else
							return EQUAL;
			case '<' :	if (peekch() == '=') {
							nextch();
							return LESSEQUAL;
						} else if (peekch() == '>') {
							nextch();
							return NOTEQUAL;
						} else
							return LESS;
			case '>' :	if (peekch() == '=') {
							nextch();
							return GREATEREQUAL;
						} else
							return GREATER;
		}
	}
	return UNKNOWN;
}


/* Token scanner API and storage for run-time data, plus the initial settings.
 *
 */
Scanner scanner = {
	.module = NULL,

	.token = UNKNOWN,
	.peeked = 0,
	.at_bol = true,
	.string[0] = 0,

	.indentlevel = 0,
	.indentation[0] = 0,

	.next = next_token,
	.peek = peek_token,
	.init = scanner_init,
	.save = scanner_save,
	.load = scanner_load
	};
