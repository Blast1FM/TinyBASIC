//Token handling functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

// ѕриватные данные
typedef struct {
    TokenClass class; // класс токена 
    int line; // номер строки, на которой был найден токен 
    int pos; // позици€ в строке, на которой был найден токен 
    char* content; // представление токена 
} Private;


 // ¬спомогательные переменные 
static Token* this; // объект токена 
static Private* data; // приватные данные 

// ѕолучение класса токена  
static TokenClass get_class(Token* token) {
    this = token;
    data = token->data;
    return data->class;
}

// ѕолучение номера строки, на которой токен начинаетс€
static int get_line(Token* token) {
    this = token;
    data = token->data;
    return data->line;
}

//ѕолучение позиции символа, на котором токен начинаетс€
static int get_pos(Token* token) {
    this = token;
    data = token->data;
    return data->pos;
}

// ѕолучение содержимого токена
static char* get_content(Token* token) {
    this = token;
    data = token->data;
    return data->content;
}

// ”становка класса токена
static void set_class(Token* token, TokenClass class) {
    this = token;
    data = this->data;
    data->class = class;
}

// ”становка номера строки и позиции дл€ токена
static void set_line_pos(Token* token, int line, int pos) {
    this = token;
    data = this->data;
    data->line = line;
    data->pos = pos;;
}

// ”становка текстового содержимого токена
static void set_content(Token* token, char* content) {
    this = token;
    data = this->data;
    if (data->content)
        free(data->content);
    data->content = malloc(strlen(content) + 1);
    strcpy(data->content, content);
}

// ”становка всех значений существующего токена одним вызовом функции.
static void initialise(Token* token, TokenClass class, int line, int pos,
    char* content) {

    // вспомогательные переменные 
    this = token;
    data = this->data;

    // инициализаци€ простых полей 
    data->class = class ? class : TOKEN_NONE;
    data->line = line ? line : 0;
    data->pos = pos ? pos : 0;

    // инициализаци€ содержимого 
    if (content)
        set_content(this, content);
}

//ƒеструктор токена
static void destroy(Token* token) {
    if ((this = token)) {
        data = this->data;
        if (data->content)
            free(data->content);
        free(data);
        free(this);
    }
}

//  онструктор токена без значений дл€ инициализации
Token* new_Token(void) {

    // создание структуры 
    this = malloc(sizeof(Token));
    this->data = data = malloc(sizeof(Private));

    // инициализаци€ свойств 
    data->class = TOKEN_NONE;
    data->line = data->pos = 0;
    data->content = NULL;
    // инициализаци€ методов
    this->initialise = initialise;
    this->get_class = get_class;
    this->get_line = get_line;
    this->get_pos = get_pos;
    this->get_content = get_content;
    this->set_class = set_class;
    this->set_line_pos = set_line_pos;
    this->set_content = set_content;
    this->destroy = destroy;

    // возвращаем созданную структуру
    return this;
}

//  онструктор токена со значени€ми дл€ инициализации
Token* new_Token_init(TokenClass class, int line, int pos, char* content) {
    this = new_Token();
    this->initialise(this, class, line, pos, content);

    // возвращем новый токен
    return this;
}