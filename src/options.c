// Модуль опций языка Tiny BASIC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"

typedef struct {
    LineNumberOption line_numbers; // обязательные, подразумеваемые, опциональные
    int line_limit; // максимальный номер строки, разрешенный
    CommentOption comments; // включены, отключены
    int gosub_limit; // количество вложенных gosubs
} Private;

static LanguageOptions* this; // объект, над которым работаем
static Private* data; // приватные данные объекта

 //Установка индивидуальной опции номера строки 
static void set_line_numbers(LanguageOptions* options, LineNumberOption line_numbers) {
    this = options;
    data = this->data;
    data->line_numbers = line_numbers;
}

//Установка индивидуальной ограничения номера строки 
static void set_line_limit(LanguageOptions* options, int line_limit) {
    this = options;
    data = this->data;
    data->line_limit = line_limit;
}

//Установка индивидуальной опции комментария 
static void set_comments(LanguageOptions* options, CommentOption comments) {
    this = options;
    data = this->data;
    data->comments = comments;
}

// Установка ограничения стека GOSUB
static void set_gosub_limit(LanguageOptions* options, int gosub_limit) {
    this = options;
    data = this->data;
    data->gosub_limit = gosub_limit;
}

// Возврат установки номера строки
static LineNumberOption get_line_numbers(LanguageOptions* options) {
    this = options;
    data = this->data;
    return data->line_numbers;
}

// Возврат ограничения количества строки
static int get_line_limit(LanguageOptions* options) {
    this = options;
    data = this->data;
    return data->line_limit;
}

//Возврат установки комментария
static CommentOption get_comments(LanguageOptions* options) {
    this = options;
    data = this->data;
    return data->comments;
}

// Возврат установки предела стека GOSUB
static int get_gosub_limit(LanguageOptions* options) {
    this = options;
    data = this->data;
    return data->gosub_limit;
}

//Уничтожение объекта настроек
static void destroy(LanguageOptions* options) {
    if (options) {
        if (options->data)
            free(options->data);
        free(options);
    }
}

//Конструктор опций языка
LanguageOptions* new_LanguageOptions(void) {

    /* выделение памяти */
    this = malloc(sizeof(LanguageOptions));
    data = this->data = malloc(sizeof(Private));

    /* инициализация методов */
    this->set_line_numbers = set_line_numbers;
    this->set_line_limit = set_line_limit;
    this->set_comments = set_comments;
    this->set_gosub_limit = set_gosub_limit;
    this->get_line_numbers = get_line_numbers;
    this->get_line_limit = get_line_limit;
    this->get_comments = get_comments;
    this->get_gosub_limit = get_gosub_limit;
    this->destroy = destroy;

    /* инициализация свойств */
    data->line_numbers = LINE_NUMBERS_OPTIONAL;
    data->line_limit = 32767;
    data->comments = COMMENTS_ENABLED;
    data->gosub_limit = 64;

    /* возврат нового объекта */
    return this;
}
