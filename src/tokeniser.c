// Tokenisation module.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "tokeniser.h"
#include "common.h"


// Режимы чтения. 
typedef enum {
    DEFAULT_MODE, // режим по умолчанию, когда мы не знаем, что будет считываться
    COMMENT_MODE, // режим чтения комментария
    WORD_MODE, //режим чтения идентификатора или ключевого слова
    NUMBER_MODE, // режим чтения числовой константы
    LESS_THAN_MODE, // режим чтения оператора, начинающегося с <
    GREATER_THAN_MODE, // режим чтения оператора, начинающегося с >
    STRING_LITERAL_MODE, // режим чтения строковой литеры
    UNKNOWN_MODE // потерянный режим
} Mode;

// Информация о текущем состоянии.
typedef struct {
    Token* token; // токен для возврата
    Mode mode; // текущий режим чтения
    int ch; // последний считанный символ
    char* content; // содержимое создаваемого токена
    int max; // выделенная память для содержимого
} TokeniserState;

// Приватные данные
typedef struct {
    FILE* input; // входной файл
    int line, // текущая строка во входном файле
        pos, // текущая позиция в строке ввода
        start_line, // строка, на которой начался токен
        start_pos; // позиция, на которой начался токен
} Private;


// Вспомогательные переменные. 
static TokenStream *this; /* token stream passed in to public method */
static Private *data; /* private data for this */

//Считываем символ и обновляем позицию
static int read_character(TokeniserState* state) {
    int ch; // символ, считанный из потока
    // считываем символ
    ch = fgetc(data->input);

    // обновляем позицию и строку
    if (ch == '\n') { // если символ - новая строка
        ++data->line; // увеличиваем счетчик строки
        data->pos = 0; // сбрасываем счетчик позиции
    }
    else {
        ++data->pos; // увеличиваем счетчик позиции
    }

    // возвращаем символ
    return ch;
}

// Вставляем символ обратно во входной поток и обновляем положение
static void unread_character (TokeniserState *state) {
  ungetc (state->ch, data->input); // возвращаем символ в поток ввода
  if (state->ch == '\n') // если символ - новая строка
      --data->line; // уменьшаем счетчик строки
  else 
      --data->pos; // уменьшаем счетчик позиции
}

// Добавляем последний прочитанный символ к содержимому токена
static void store_character(TokeniserState* state) {
    char* temp; // временный указатель на содержимое
    int length; // текущая длина токена 

    // выделяем больше памяти для содержимого токена, если это необходимо
    if (strlen(state->content) == state->max - 1) {
        temp = state->content; // временно сохраняем указатель на содержимое
        state->max *= 2; // увеличиваем выделенную память в два раза
        state->content = malloc(state->max); // выделяем новую память для содержимого
        strcpy(state->content, temp); // копируем содержимое в новую память
        free(temp); // освобождаем старую память
    }

    // добавляем символ к токену
    length = strlen(state->content); // получаем текущую длину содержимого
    state->content[length++] = state->ch; // добавляем символ к содержимому
    state->content[length] = '\0'; // добавляем завершающий нулевой символ
}


// Идентифицируем распознанные символы
static TokenClass identify_symbol(int ch) {
    switch (ch) {
    case '+':
        return TOKEN_PLUS; // возвращаем класс токена "плюс"
        break;
    case '-':
        return TOKEN_MINUS; // возвращаем класс токена "минус"
        break;
    case '*':
        return TOKEN_MULTIPLY; // возвращаем класс токена "умножить"
        break;
    case '/':
        return TOKEN_DIVIDE; // возвращаем класс токена "разделить"
        break;
    case '=':
        return TOKEN_EQUAL; // возвращаем класс токена "равно"
        break;
    case '(':
        return TOKEN_LEFT_PARENTHESIS; // возвращаем класс токена "левая скобка"
        break;
    case ')':
        return TOKEN_RIGHT_PARENTHESIS; // возвращаем класс токена "правая скобка"
        break;
    case ',':
        return TOKEN_COMMA; // возвращаем класс токена "запятая"
        break;
    case ';':
        return TOKEN_SEMICOLON; // возвращаем класс токена "точка с запятой"
        break;
    default:
        return TOKEN_SYMBOL; // возвращаем класс токена "символ"
    }
}

static TokenClass identify_word(char* word) {
    if (strlen(word) == 1)
        return TOKEN_VARIABLE; // возвращаем класс токена "переменная"
    else if (!tinybasic_strcmp(word, "LET"))
        return TOKEN_LET; // возвращаем класс токена "LET"
    else if (!tinybasic_strcmp(word, "IF"))
        return TOKEN_IF; // возвращаем класс токена "IF"
    else if (!tinybasic_strcmp(word, "THEN"))
        return TOKEN_THEN; // возвращаем класс токена "THEN"
    else if (!tinybasic_strcmp(word, "GOTO"))
        return TOKEN_GOTO; // возвращаем класс токена "GOTO"
    else if (!tinybasic_strcmp(word, "GOSUB"))
        return TOKEN_GOSUB; // возвращаем класс токена "GOSUB"
    else if (!tinybasic_strcmp(word, "RETURN"))
        return TOKEN_RETURN; // возвращаем класс токена "RETURN"
    else if (!tinybasic_strcmp(word, "END"))
        return TOKEN_END; // возвращаем класс токена "END"
    else if (!tinybasic_strcmp(word, "PRINT"))
        return TOKEN_PRINT; // возвращаем класс токена "PRINT"
    else if (!tinybasic_strcmp(word, "INPUT"))
        return TOKEN_INPUT; // возвращаем класс токена "INPUT"
    else if (!tinybasic_strcmp(word, "REM"))
        return TOKEN_REM; // возвращаем класс токена "REM"
    else if (!tinybasic_strcmp(word, "RND"))
        return TOKEN_RND; // возвращаем класс токена "RND"
    else
        return TOKEN_WORD; // возвращаем класс токена "слово"
}

// Идентифицируем составные символы
static TokenClass identify_compound_symbol(char* symbol) {
    if (!strcmp(symbol, "<>") // если символ - "<>"
        || !strcmp(symbol, "><")) // или символ - "><"
        return TOKEN_UNEQUAL; // возвращаем класс токена "не равно"
    else if (!strcmp(symbol, "<"))
        return TOKEN_LESSTHAN; // возвращаем класс токена "меньше"
    else if (!strcmp(symbol, "<="))
        return TOKEN_LESSOREQUAL; // возвращаем класс токена "меньше или равно"
    else if (!strcmp(symbol, ">"))
        return TOKEN_GREATERTHAN; // возвращаем класс токена "больше"
    else if (!strcmp(symbol, ">="))
        return TOKEN_GREATEROREQUAL; // возвращаем класс токена "больше или равно"
    else
        return TOKEN_SYMBOL; // возвращаем класс токена "символ"
}

static void default_mode(TokeniserState* state) {

    // обработка пробелов и символов табуляции, не являющихся концом строки
    if (state->ch == ' '
        state->ch == '\t') {
        state->ch = read_character(state); // считываем следующий символ
        data->start_line = data->line; // сохраняем номер строки начала токена
        data->start_pos = data->pos; // сохраняем позицию начала токена
    }

    // обработка пробелов в конце строки
    else if (state->ch == '\n') {
        data->start_line = data->line - 1; // сохраняем номер предыдущей строки
        data->start_pos = data->pos; // сохраняем позицию начала токена
        state->token = new_Token_init
        (TOKEN_EOL, data->start_line, data->start_pos, state->content); // создаем токен для конца строки
    }

    // обработка слов
    else if ((state->ch >= 'A' && state->ch <= 'Z')
        (state->ch >= 'a' && state->ch <= 'z')) {
        data->start_line = data->line; // сохраняем номер строки начала токена
        data->start_pos = data->pos; // сохраняем позицию начала токена
        state->mode = WORD_MODE; // переходим в режим чтения слова
    }

    // обработка чисел
    else if (state->ch >= '0' && state->ch <= '9')
        state->mode = NUMBER_MODE; // переходим в режим чтения числа

    //проверка на операторы, начинающиеся с < (<, <=, <>)
    else if (state->ch == '<') {
        data->start_line = data->line; // сохраняем номер строки начала токена
        data->start_pos = data->pos; // сохраняем позицию начала токена
        store_character(state); // сохраняем символ <
        state->ch = read_character(state); // считываем следующий символ
        state->mode = LESS_THAN_MODE; // переходим в режим чтения оператора, начинающегося с <
    }

    // проверка на операторы, начинающиеся с >
    else if (state->ch == '>') {
        data->start_line = data->line; // сохраняем номер строки начала токена
        data->start_pos = data->pos; // сохраняем позицию начала токена
        store_character(state); // сохраняем символ >
        state->ch = read_character(state); // считываем следующий символ
        state->mode = GREATER_THAN_MODE; // переходим в режим чтения оператора, начинающегося с >
    }

    // обработка других операторов-символов
    else if (strchr("+-*/=(),;", state->ch) != NULL) {
        data->start_line = data->line; // сохраняем номер строки начала токена
        data->start_pos = data->pos; // сохраняем позицию начала токена
        store_character(state); // сохраняем символ
        state->token = new_Token_init(identify_symbol(state->ch),
            data->start_line, data->start_pos, state->content); // создаем токен для символа-оператора
    }

    // обработка двойных ковычек
    else if (state->ch == '"') {
        data->start_line = data->line; // сохраняем номер строки начала токена
        data->start_pos = data->pos; // сохраняем позицию начала токена
        state->ch = read_character(state); // считываем следующий символ
        state->mode = STRING_LITERAL_MODE; // переходим в режим чтения строковой литеры
    }

    // пришли в конец файла
    else if (state->ch == EOF) {
        data->start_line = data->line; // сохраняем номер строки начала токена
        data->start_pos = data->pos; // сохраняем позицию начала токена
        state->token = new_Token_init
        (TOKEN_EOF, data->start_line, data->start_pos, state->content); // создаем токен для конца файла
    }

    // обработка недопустимых символов 
    else {
        data->start_line = data->line; // сохраняем номер строки начала токена
        data->start_pos = data->pos; // сохраняем позицию начала токена
        store_character(state); // сохраняем символ
        state->token = new_Token_init
        (TOKEN_ILLEGAL, data->start_line, data->start_pos, state->content); // создаем токен для недопустимого символа
    }
}   

// Режим чтения слова - обрабатывает символы при создании токена слова
static void word_mode(TokeniserState* state) {

    TokenClass class; // распознанный класс ключевого слова

    // добавляем буквы и цифры к токену 
    if ((state->ch >= 'A' && state->ch <= 'Z') ||
        (state->ch >= 'a' && state->ch <= 'z')) {
        store_character(state); // сохраняем символ
        state->ch = read_character(state); // считываем следующий символ
    }

    // остальные символы сохраняются для следующего токена 
    else {
        if (state->ch != EOF)
            unread_character(state); // возврат символа в поток ввода
        class = identify_word(state->content); // определение класса токена слова
        if (class == TOKEN_REM) {
            *state->content = '\0'; // обнуляем содержимое токена
            state->mode = COMMENT_MODE; // переходим в режим чтения комментария
        }
        else
            state->token = new_Token_init
            (class, data->start_line, data->start_pos, state->content); // создаем токен
    }
}

// Режим чтения комментария - пропуск до конца строки после REM
static void comment_mode(TokeniserState* state) {
    if (state->ch == '\n')
        state->mode = DEFAULT_MODE; // возвращаемся в режим по умолчанию
    else
        state->ch = read_character(state); // считываем следующий символ
}

// Режим чтения числа - создание токена числа (только целого)
static void number_mode(TokeniserState* state) {

    // добавляем цифры к токену 
    if (state->ch >= '0' && state->ch <= '9') {
        store_character(state); // сохраняем символ
        state->ch = read_character(state); // считываем следующий символ
    }

    // остальные символы сохраняются для следующего токена
    else {
        if (state->ch != EOF)
            unread_character(state); // возврат символа в поток ввода
        state->token = new_Token_init
        (TOKEN_NUMBER, data->start_line, data->start_pos, state->content); // создаем токен
    }

}


/*
 * Режим чтения операторов, начинающихся с "<" - проверка на <> и <=
 * Глобальные переменные:
 *   int               start_line   номер строки, на которой начался текущий токен
 *   int               start_pos    позиция символа, на котором начался текущий токен
 * Параметры:
 *   TokeniserState*   state        текущее состояние токенизатора
 */
static void less_than_mode(TokeniserState* state) {
    if (state->ch == '=' || state->ch == '>')
        store_character(state); // сохраняем символ
    else
        unread_character(state); // возвращаем символ в поток ввода
    state->token = new_Token_init
    (identify_compound_symbol(state->content), data->start_line,
        data->start_pos, state->content); // создаем токен
}

// Режим чтения операторов, начинающихся с ">" - проверка на >= и ><
static void greater_than_mode(TokeniserState* state) {
    if (state->ch == '=' || state->ch == '<')
        store_character(state); // сохраняем символ
    else
        ungetc(state->ch, data->input); // возвращаем символ в поток ввода
    state->token = new_Token_init
    (identify_compound_symbol(state->content), data->start_line,
        data->start_pos, state->content); // создаем токен
}

// Режим чтения строки
static void string_literal_mode(TokeniserState* state) {

    // кавычка завершает строку 
    if (state->ch == '"')
        state->token = new_Token_init
        (TOKEN_STRING, data->start_line, data->start_pos, state->content); // создаем токен для строковой литеры

    // два слеша завершают строку
    else if (state->ch == '\\') {
        state->ch = read_character(state); // считываем следующий символ
        store_character(state); // сохраняем символ
        state->ch = read_character(state); // считываем следующий символ
    }

    // конец файла генерирует ошибку 
    else if (state->ch == EOF)
        state->token = new_Token_init
        (TOKEN_ILLEGAL, data->start_line, data->start_pos, state->content); // создаем токен для недопустимого символа

    // все остальные символы являются частью строки 
    else {
        store_character(state); // сохраняем символ
        state->ch = read_character(state); // считываем следующий символ
    }
}


// Получаем следующий токен
static Token* next(TokenStream* token_stream) {

    TokeniserState state; // текущее состояние чтения
    Token* return_token; // токен для возврата 

    // инициализация 
    this = token_stream;
    data = this->data;
    state.token = NULL;
    state.mode = DEFAULT_MODE;
    state.max = 1024;
    state.content = malloc(state.max);
    *(state.content) = '\0';
    state.ch = read_character(&state); // считываем первый символ

    while (state.token == NULL) {
        switch (state.mode) {
        case DEFAULT_MODE:
            default_mode(&state); // режим по умолчанию
            break;
        case COMMENT_MODE:
            comment_mode(&state); // режим комментария
            break;
        case WORD_MODE:
            word_mode(&state); // режим чтения слова
            break;
        case NUMBER_MODE:
            number_mode(&state); // режим чтения числа
            break;
        case LESS_THAN_MODE:
            less_than_mode(&state); // режим чтения оператора, начинающегося с <
            break;
        case GREATER_THAN_MODE:
            greater_than_mode(&state); // режим чтения оператора, начинающегося с >
            break;
        case STRING_LITERAL_MODE:
            string_literal_mode(&state); // режим чтения строковой литеры
            break;
        default:
            state.token = new_Token_init
            (TOKEN_EOF, data->start_line, data->start_pos, state.content); // встречен конец файла
            state.ch = EOF; 
        }
    }

    //сохраняем токен и освобождаем память состояния 
    return_token = state.token;
    free(state.content);

    // возвращаем результат 
    return return_token;
}

// Получение текущего номера строки
static int get_line(TokenStream* token_stream) {
    this = token_stream;
    data = this->data;
    return data->line;
}

// Деструктор для TokenStream
static void destroy(TokenStream* token_stream) {
    if (token_stream) {
        if (token_stream->data)
            free(token_stream->data);
        free(token_stream);
    }
}

//Конструктор для TokenStream
TokenStream* new_TokenStream(FILE* input) {

    //выделение памяти 
    this = malloc(sizeof(TokenStream));
    this->data = data = malloc(sizeof(Private));

    // инициализация методов 
    this->next = next;
    this->get_line = get_line;
    this->destroy = destroy;

    // инициализация данных 
    data->input = input;
    data->line = data->start_line = 1;
    data->pos = data->start_pos = 0;

    // возвращение нового токенового потока 
    return this;
}