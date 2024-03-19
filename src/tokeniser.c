// Tokenisation module.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "tokeniser.h"
#include "common.h"


// ������ ������. 
typedef enum {
    DEFAULT_MODE, // ����� �� ���������, ����� �� �� �����, ��� ����� �����������
    COMMENT_MODE, // ����� ������ �����������
    WORD_MODE, //����� ������ �������������� ��� ��������� �����
    NUMBER_MODE, // ����� ������ �������� ���������
    LESS_THAN_MODE, // ����� ������ ���������, ������������� � <
    GREATER_THAN_MODE, // ����� ������ ���������, ������������� � >
    STRING_LITERAL_MODE, // ����� ������ ��������� ������
    UNKNOWN_MODE // ���������� �����
} Mode;

// ���������� � ������� ���������.
typedef struct {
    Token* token; // ����� ��� ��������
    Mode mode; // ������� ����� ������
    int ch; // ��������� ��������� ������
    char* content; // ���������� ������������ ������
    int max; // ���������� ������ ��� �����������
} TokeniserState;

// ��������� ������
typedef struct {
    FILE* input; // ������� ����
    int line, // ������� ������ �� ������� �����
        pos, // ������� ������� � ������ �����
        start_line, // ������, �� ������� ������� �����
        start_pos; // �������, �� ������� ������� �����
} Private;


// ��������������� ����������. 
static TokenStream *this; /* token stream passed in to public method */
static Private *data; /* private data for this */

//��������� ������ � ��������� �������
static int read_character(TokeniserState* state) {
    int ch; // ������, ��������� �� ������
    // ��������� ������
    ch = fgetc(data->input);

    // ��������� ������� � ������
    if (ch == '\n') { // ���� ������ - ����� ������
        ++data->line; // ����������� ������� ������
        data->pos = 0; // ���������� ������� �������
    }
    else {
        ++data->pos; // ����������� ������� �������
    }

    // ���������� ������
    return ch;
}

// ��������� ������ ������� �� ������� ����� � ��������� ���������
static void unread_character (TokeniserState *state) {
  ungetc (state->ch, data->input); // ���������� ������ � ����� �����
  if (state->ch == '\n') // ���� ������ - ����� ������
      --data->line; // ��������� ������� ������
  else 
      --data->pos; // ��������� ������� �������
}

// ��������� ��������� ����������� ������ � ����������� ������
static void store_character(TokeniserState* state) {
    char* temp; // ��������� ��������� �� ����������
    int length; // ������� ����� ������ 

    // �������� ������ ������ ��� ����������� ������, ���� ��� ����������
    if (strlen(state->content) == state->max - 1) {
        temp = state->content; // �������� ��������� ��������� �� ����������
        state->max *= 2; // ����������� ���������� ������ � ��� ����
        state->content = malloc(state->max); // �������� ����� ������ ��� �����������
        strcpy(state->content, temp); // �������� ���������� � ����� ������
        free(temp); // ����������� ������ ������
    }

    // ��������� ������ � ������
    length = strlen(state->content); // �������� ������� ����� �����������
    state->content[length++] = state->ch; // ��������� ������ � �����������
    state->content[length] = '\0'; // ��������� ����������� ������� ������
}


// �������������� ������������ �������
static TokenClass identify_symbol(int ch) {
    switch (ch) {
    case '+':
        return TOKEN_PLUS; // ���������� ����� ������ "����"
        break;
    case '-':
        return TOKEN_MINUS; // ���������� ����� ������ "�����"
        break;
    case '*':
        return TOKEN_MULTIPLY; // ���������� ����� ������ "��������"
        break;
    case '/':
        return TOKEN_DIVIDE; // ���������� ����� ������ "���������"
        break;
    case '=':
        return TOKEN_EQUAL; // ���������� ����� ������ "�����"
        break;
    case '(':
        return TOKEN_LEFT_PARENTHESIS; // ���������� ����� ������ "����� ������"
        break;
    case ')':
        return TOKEN_RIGHT_PARENTHESIS; // ���������� ����� ������ "������ ������"
        break;
    case ',':
        return TOKEN_COMMA; // ���������� ����� ������ "�������"
        break;
    case ';':
        return TOKEN_SEMICOLON; // ���������� ����� ������ "����� � �������"
        break;
    default:
        return TOKEN_SYMBOL; // ���������� ����� ������ "������"
    }
}

static TokenClass identify_word(char* word) {
    if (strlen(word) == 1)
        return TOKEN_VARIABLE; // ���������� ����� ������ "����������"
    else if (!tinybasic_strcmp(word, "LET"))
        return TOKEN_LET; // ���������� ����� ������ "LET"
    else if (!tinybasic_strcmp(word, "IF"))
        return TOKEN_IF; // ���������� ����� ������ "IF"
    else if (!tinybasic_strcmp(word, "THEN"))
        return TOKEN_THEN; // ���������� ����� ������ "THEN"
    else if (!tinybasic_strcmp(word, "GOTO"))
        return TOKEN_GOTO; // ���������� ����� ������ "GOTO"
    else if (!tinybasic_strcmp(word, "GOSUB"))
        return TOKEN_GOSUB; // ���������� ����� ������ "GOSUB"
    else if (!tinybasic_strcmp(word, "RETURN"))
        return TOKEN_RETURN; // ���������� ����� ������ "RETURN"
    else if (!tinybasic_strcmp(word, "END"))
        return TOKEN_END; // ���������� ����� ������ "END"
    else if (!tinybasic_strcmp(word, "PRINT"))
        return TOKEN_PRINT; // ���������� ����� ������ "PRINT"
    else if (!tinybasic_strcmp(word, "INPUT"))
        return TOKEN_INPUT; // ���������� ����� ������ "INPUT"
    else if (!tinybasic_strcmp(word, "REM"))
        return TOKEN_REM; // ���������� ����� ������ "REM"
    else if (!tinybasic_strcmp(word, "RND"))
        return TOKEN_RND; // ���������� ����� ������ "RND"
    else
        return TOKEN_WORD; // ���������� ����� ������ "�����"
}

// �������������� ��������� �������
static TokenClass identify_compound_symbol(char* symbol) {
    if (!strcmp(symbol, "<>") // ���� ������ - "<>"
        || !strcmp(symbol, "><")) // ��� ������ - "><"
        return TOKEN_UNEQUAL; // ���������� ����� ������ "�� �����"
    else if (!strcmp(symbol, "<"))
        return TOKEN_LESSTHAN; // ���������� ����� ������ "������"
    else if (!strcmp(symbol, "<="))
        return TOKEN_LESSOREQUAL; // ���������� ����� ������ "������ ��� �����"
    else if (!strcmp(symbol, ">"))
        return TOKEN_GREATERTHAN; // ���������� ����� ������ "������"
    else if (!strcmp(symbol, ">="))
        return TOKEN_GREATEROREQUAL; // ���������� ����� ������ "������ ��� �����"
    else
        return TOKEN_SYMBOL; // ���������� ����� ������ "������"
}

static void default_mode(TokeniserState* state) {

    // ��������� �������� � �������� ���������, �� ���������� ������ ������
    if (state->ch == ' '
        state->ch == '\t') {
        state->ch = read_character(state); // ��������� ��������� ������
        data->start_line = data->line; // ��������� ����� ������ ������ ������
        data->start_pos = data->pos; // ��������� ������� ������ ������
    }

    // ��������� �������� � ����� ������
    else if (state->ch == '\n') {
        data->start_line = data->line - 1; // ��������� ����� ���������� ������
        data->start_pos = data->pos; // ��������� ������� ������ ������
        state->token = new_Token_init
        (TOKEN_EOL, data->start_line, data->start_pos, state->content); // ������� ����� ��� ����� ������
    }

    // ��������� ����
    else if ((state->ch >= 'A' && state->ch <= 'Z')
        (state->ch >= 'a' && state->ch <= 'z')) {
        data->start_line = data->line; // ��������� ����� ������ ������ ������
        data->start_pos = data->pos; // ��������� ������� ������ ������
        state->mode = WORD_MODE; // ��������� � ����� ������ �����
    }

    // ��������� �����
    else if (state->ch >= '0' && state->ch <= '9')
        state->mode = NUMBER_MODE; // ��������� � ����� ������ �����

    //�������� �� ���������, ������������ � < (<, <=, <>)
    else if (state->ch == '<') {
        data->start_line = data->line; // ��������� ����� ������ ������ ������
        data->start_pos = data->pos; // ��������� ������� ������ ������
        store_character(state); // ��������� ������ <
        state->ch = read_character(state); // ��������� ��������� ������
        state->mode = LESS_THAN_MODE; // ��������� � ����� ������ ���������, ������������� � <
    }

    // �������� �� ���������, ������������ � >
    else if (state->ch == '>') {
        data->start_line = data->line; // ��������� ����� ������ ������ ������
        data->start_pos = data->pos; // ��������� ������� ������ ������
        store_character(state); // ��������� ������ >
        state->ch = read_character(state); // ��������� ��������� ������
        state->mode = GREATER_THAN_MODE; // ��������� � ����� ������ ���������, ������������� � >
    }

    // ��������� ������ ����������-��������
    else if (strchr("+-*/=(),;", state->ch) != NULL) {
        data->start_line = data->line; // ��������� ����� ������ ������ ������
        data->start_pos = data->pos; // ��������� ������� ������ ������
        store_character(state); // ��������� ������
        state->token = new_Token_init(identify_symbol(state->ch),
            data->start_line, data->start_pos, state->content); // ������� ����� ��� �������-���������
    }

    // ��������� ������� �������
    else if (state->ch == '"') {
        data->start_line = data->line; // ��������� ����� ������ ������ ������
        data->start_pos = data->pos; // ��������� ������� ������ ������
        state->ch = read_character(state); // ��������� ��������� ������
        state->mode = STRING_LITERAL_MODE; // ��������� � ����� ������ ��������� ������
    }

    // ������ � ����� �����
    else if (state->ch == EOF) {
        data->start_line = data->line; // ��������� ����� ������ ������ ������
        data->start_pos = data->pos; // ��������� ������� ������ ������
        state->token = new_Token_init
        (TOKEN_EOF, data->start_line, data->start_pos, state->content); // ������� ����� ��� ����� �����
    }

    // ��������� ������������ �������� 
    else {
        data->start_line = data->line; // ��������� ����� ������ ������ ������
        data->start_pos = data->pos; // ��������� ������� ������ ������
        store_character(state); // ��������� ������
        state->token = new_Token_init
        (TOKEN_ILLEGAL, data->start_line, data->start_pos, state->content); // ������� ����� ��� ������������� �������
    }
}   

// ����� ������ ����� - ������������ ������� ��� �������� ������ �����
static void word_mode(TokeniserState* state) {

    TokenClass class; // ������������ ����� ��������� �����

    // ��������� ����� � ����� � ������ 
    if ((state->ch >= 'A' && state->ch <= 'Z') ||
        (state->ch >= 'a' && state->ch <= 'z')) {
        store_character(state); // ��������� ������
        state->ch = read_character(state); // ��������� ��������� ������
    }

    // ��������� ������� ����������� ��� ���������� ������ 
    else {
        if (state->ch != EOF)
            unread_character(state); // ������� ������� � ����� �����
        class = identify_word(state->content); // ����������� ������ ������ �����
        if (class == TOKEN_REM) {
            *state->content = '\0'; // �������� ���������� ������
            state->mode = COMMENT_MODE; // ��������� � ����� ������ �����������
        }
        else
            state->token = new_Token_init
            (class, data->start_line, data->start_pos, state->content); // ������� �����
    }
}

// ����� ������ ����������� - ������� �� ����� ������ ����� REM
static void comment_mode(TokeniserState* state) {
    if (state->ch == '\n')
        state->mode = DEFAULT_MODE; // ������������ � ����� �� ���������
    else
        state->ch = read_character(state); // ��������� ��������� ������
}

// ����� ������ ����� - �������� ������ ����� (������ ������)
static void number_mode(TokeniserState* state) {

    // ��������� ����� � ������ 
    if (state->ch >= '0' && state->ch <= '9') {
        store_character(state); // ��������� ������
        state->ch = read_character(state); // ��������� ��������� ������
    }

    // ��������� ������� ����������� ��� ���������� ������
    else {
        if (state->ch != EOF)
            unread_character(state); // ������� ������� � ����� �����
        state->token = new_Token_init
        (TOKEN_NUMBER, data->start_line, data->start_pos, state->content); // ������� �����
    }

}


/*
 * ����� ������ ����������, ������������ � "<" - �������� �� <> � <=
 * ���������� ����������:
 *   int               start_line   ����� ������, �� ������� ������� ������� �����
 *   int               start_pos    ������� �������, �� ������� ������� ������� �����
 * ���������:
 *   TokeniserState*   state        ������� ��������� ������������
 */
static void less_than_mode(TokeniserState* state) {
    if (state->ch == '=' || state->ch == '>')
        store_character(state); // ��������� ������
    else
        unread_character(state); // ���������� ������ � ����� �����
    state->token = new_Token_init
    (identify_compound_symbol(state->content), data->start_line,
        data->start_pos, state->content); // ������� �����
}

// ����� ������ ����������, ������������ � ">" - �������� �� >= � ><
static void greater_than_mode(TokeniserState* state) {
    if (state->ch == '=' || state->ch == '<')
        store_character(state); // ��������� ������
    else
        ungetc(state->ch, data->input); // ���������� ������ � ����� �����
    state->token = new_Token_init
    (identify_compound_symbol(state->content), data->start_line,
        data->start_pos, state->content); // ������� �����
}

// ����� ������ ������
static void string_literal_mode(TokeniserState* state) {

    // ������� ��������� ������ 
    if (state->ch == '"')
        state->token = new_Token_init
        (TOKEN_STRING, data->start_line, data->start_pos, state->content); // ������� ����� ��� ��������� ������

    // ��� ����� ��������� ������
    else if (state->ch == '\\') {
        state->ch = read_character(state); // ��������� ��������� ������
        store_character(state); // ��������� ������
        state->ch = read_character(state); // ��������� ��������� ������
    }

    // ����� ����� ���������� ������ 
    else if (state->ch == EOF)
        state->token = new_Token_init
        (TOKEN_ILLEGAL, data->start_line, data->start_pos, state->content); // ������� ����� ��� ������������� �������

    // ��� ��������� ������� �������� ������ ������ 
    else {
        store_character(state); // ��������� ������
        state->ch = read_character(state); // ��������� ��������� ������
    }
}


// �������� ��������� �����
static Token* next(TokenStream* token_stream) {

    TokeniserState state; // ������� ��������� ������
    Token* return_token; // ����� ��� �������� 

    // ������������� 
    this = token_stream;
    data = this->data;
    state.token = NULL;
    state.mode = DEFAULT_MODE;
    state.max = 1024;
    state.content = malloc(state.max);
    *(state.content) = '\0';
    state.ch = read_character(&state); // ��������� ������ ������

    while (state.token == NULL) {
        switch (state.mode) {
        case DEFAULT_MODE:
            default_mode(&state); // ����� �� ���������
            break;
        case COMMENT_MODE:
            comment_mode(&state); // ����� �����������
            break;
        case WORD_MODE:
            word_mode(&state); // ����� ������ �����
            break;
        case NUMBER_MODE:
            number_mode(&state); // ����� ������ �����
            break;
        case LESS_THAN_MODE:
            less_than_mode(&state); // ����� ������ ���������, ������������� � <
            break;
        case GREATER_THAN_MODE:
            greater_than_mode(&state); // ����� ������ ���������, ������������� � >
            break;
        case STRING_LITERAL_MODE:
            string_literal_mode(&state); // ����� ������ ��������� ������
            break;
        default:
            state.token = new_Token_init
            (TOKEN_EOF, data->start_line, data->start_pos, state.content); // �������� ����� �����
            state.ch = EOF; 
        }
    }

    //��������� ����� � ����������� ������ ��������� 
    return_token = state.token;
    free(state.content);

    // ���������� ��������� 
    return return_token;
}

// ��������� �������� ������ ������
static int get_line(TokenStream* token_stream) {
    this = token_stream;
    data = this->data;
    return data->line;
}

// ���������� ��� TokenStream
static void destroy(TokenStream* token_stream) {
    if (token_stream) {
        if (token_stream->data)
            free(token_stream->data);
        free(token_stream);
    }
}

//����������� ��� TokenStream
TokenStream* new_TokenStream(FILE* input) {

    //��������� ������ 
    this = malloc(sizeof(TokenStream));
    this->data = data = malloc(sizeof(Private));

    // ������������� ������� 
    this->next = next;
    this->get_line = get_line;
    this->destroy = destroy;

    // ������������� ������ 
    data->input = input;
    data->line = data->start_line = 1;
    data->pos = data->start_pos = 0;

    // ����������� ������ ���������� ������ 
    return this;
}