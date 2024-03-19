//Token handling functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

// ��������� ������
typedef struct {
    TokenClass class; // ����� ������ 
    int line; // ����� ������, �� ������� ��� ������ ����� 
    int pos; // ������� � ������, �� ������� ��� ������ ����� 
    char* content; // ������������� ������ 
} Private;


 // ��������������� ���������� 
static Token* this; // ������ ������ 
static Private* data; // ��������� ������ 

// ��������� ������ ������  
static TokenClass get_class(Token* token) {
    this = token;
    data = token->data;
    return data->class;
}

// ��������� ������ ������, �� ������� ����� ����������
static int get_line(Token* token) {
    this = token;
    data = token->data;
    return data->line;
}

//��������� ������� �������, �� ������� ����� ����������
static int get_pos(Token* token) {
    this = token;
    data = token->data;
    return data->pos;
}

// ��������� ����������� ������
static char* get_content(Token* token) {
    this = token;
    data = token->data;
    return data->content;
}

// ��������� ������ ������
static void set_class(Token* token, TokenClass class) {
    this = token;
    data = this->data;
    data->class = class;
}

// ��������� ������ ������ � ������� ��� ������
static void set_line_pos(Token* token, int line, int pos) {
    this = token;
    data = this->data;
    data->line = line;
    data->pos = pos;;
}

// ��������� ���������� ����������� ������
static void set_content(Token* token, char* content) {
    this = token;
    data = this->data;
    if (data->content)
        free(data->content);
    data->content = malloc(strlen(content) + 1);
    strcpy(data->content, content);
}

// ��������� ���� �������� ������������� ������ ����� ������� �������.
static void initialise(Token* token, TokenClass class, int line, int pos,
    char* content) {

    // ��������������� ���������� 
    this = token;
    data = this->data;

    // ������������� ������� ����� 
    data->class = class ? class : TOKEN_NONE;
    data->line = line ? line : 0;
    data->pos = pos ? pos : 0;

    // ������������� ����������� 
    if (content)
        set_content(this, content);
}

//���������� ������
static void destroy(Token* token) {
    if ((this = token)) {
        data = this->data;
        if (data->content)
            free(data->content);
        free(data);
        free(this);
    }
}

// ����������� ������ ��� �������� ��� �������������
Token* new_Token(void) {

    // �������� ��������� 
    this = malloc(sizeof(Token));
    this->data = data = malloc(sizeof(Private));

    // ������������� ������� 
    data->class = TOKEN_NONE;
    data->line = data->pos = 0;
    data->content = NULL;
    // ������������� �������
    this->initialise = initialise;
    this->get_class = get_class;
    this->get_line = get_line;
    this->get_pos = get_pos;
    this->get_content = get_content;
    this->set_class = set_class;
    this->set_line_pos = set_line_pos;
    this->set_content = set_content;
    this->destroy = destroy;

    // ���������� ��������� ���������
    return this;
}

// ����������� ������ �� ���������� ��� �������������
Token* new_Token_init(TokenClass class, int line, int pos, char* content) {
    this = new_Token();
    this->initialise(this, class, line, pos, content);

    // ��������� ����� �����
    return this;
}