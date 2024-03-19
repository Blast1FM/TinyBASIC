//Модуль обработки ошибок.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"


/* Приватные данные. */
typedef struct {
  // Последняя обнаруженная ошибка.
  ErrorCode error;
  // Исходная строка, в которой произошла ошибка.
  int line; 
  // Метка для исходной строки.
  int label;
} Private;


/* Вспомогательные переменные */
// Объект, над которым ведется работа
ErrorHandler *this;
// Приватные данные объекта, над которым ведется работа
Private *data; 

// Типы ошибок.
static char *messages[E_LAST] = { 
  "Successful",
  "Invalid line number",
  "Unrecognised command",
  "Invalid variable",
  "Invalid assignment",
  "Invalid expression",
  "Missing )",
  "Invalid PRINT output",
  "Bad command line",
  "File not found",
  "Invalid operator",
  "THEN expected",
  "Unexpected parameter",
  "RETURN without GOSUB",
  "Divide by zero",
  "Overflow",
  "Out of memory",
  "Too many gosubs"
};



 // Записать обнаруженную ошибку.
static void set_code(ErrorHandler* errors, ErrorCode new_error, int new_line,
    int new_label) {

    // инициализация
    this = errors;
    data = this->data;

    // установка свойств
    data->error = new_error;
    data->line = new_line;
    data->label = new_label;
}

// Вернуть метку последней ошибочной строки.
static int get_label(ErrorHandler* errors) {
    this = errors;
    data = this->data;
    return data->label;
}

//Вернуть номер последней ошибочной строки
static int get_line(ErrorHandler* errors) {
    this = errors;
    data = this->data;
    return data->line;
}

// Вернуть код последней обнаруженной ошибки.
static ErrorCode get_code(ErrorHandler* errors) {
    this = errors;
    data = this->data;
    return data->error;
}
//Сгенерировать сообщение об ошибке.
static char* get_text(ErrorHandler* errors) {

    // Локальные переменные
    char
        * message, // Полное сообщение
        * line_text, // Строка N
        * label_text; // Метка N

    // Инициализация объекта ошибки
    this = errors;
    data = this->data;

    // Получение исходной строки, если она есть
    line_text = malloc(20);
    if (data->line)
        sprintf(line_text, ", source line %d", data->line);
    else
        strcpy(line_text, "");

    // Получение метки строки, если она есть
    label_text = malloc(19);
    if (data->label)
        sprintf(label_text, ", line label %d", data->label);
    else
        strcpy(label_text, "");

    // Составление сообщения об ошибке
    message = malloc(strlen(messages[data->error]) + strlen(line_text)
        + strlen(label_text) + 1);
    strcpy(message, messages[data->error]);
    strcat(message, line_text);
    strcat(message, label_text);
    free(line_text);
    free(label_text);

    // Возврат собранного сообщения об ошибке
    return message;
}


 // Основной конструктор.
ErrorHandler* new_ErrorHandler(void) {

    // Выделение памяти
    this = malloc(sizeof(ErrorHandler));
    this->data = data = malloc(sizeof(Private));

    // Инициализация методов
    this->set_code = set_code;
    this->get_code = get_code;
    this->get_line = get_line;
    this->get_label = get_label;
    this->get_text = get_text;
    this->destroy = destroy;

    // Инициализация свойств
    data->error = E_NONE;
    data->line = 0;
    data->label = 0;

    // Возврат нового объекта
    return this;
}

// Деструктор ErrorHandler
static void destroy(ErrorHandler* errors) {
    if ((this = errors)) {
        data = this->data;
        free(data);
        free(this);
    }
}