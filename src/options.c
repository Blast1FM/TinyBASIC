// ������ ����� ����� Tiny BASIC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"

typedef struct {
    LineNumberOption line_numbers; // ������������, ���������������, ������������
    int line_limit; // ������������ ����� ������, �����������
    CommentOption comments; // ��������, ���������
    int gosub_limit; // ���������� ��������� gosubs
} Private;

static LanguageOptions* this; // ������, ��� ������� ��������
static Private* data; // ��������� ������ �������

 //��������� �������������� ����� ������ ������ 
static void set_line_numbers(LanguageOptions* options, LineNumberOption line_numbers) {
    this = options;
    data = this->data;
    data->line_numbers = line_numbers;
}

//��������� �������������� ����������� ������ ������ 
static void set_line_limit(LanguageOptions* options, int line_limit) {
    this = options;
    data = this->data;
    data->line_limit = line_limit;
}

//��������� �������������� ����� ����������� 
static void set_comments(LanguageOptions* options, CommentOption comments) {
    this = options;
    data = this->data;
    data->comments = comments;
}

// ��������� ����������� ����� GOSUB
static void set_gosub_limit(LanguageOptions* options, int gosub_limit) {
    this = options;
    data = this->data;
    data->gosub_limit = gosub_limit;
}

// ������� ��������� ������ ������
static LineNumberOption get_line_numbers(LanguageOptions* options) {
    this = options;
    data = this->data;
    return data->line_numbers;
}

// ������� ����������� ���������� ������
static int get_line_limit(LanguageOptions* options) {
    this = options;
    data = this->data;
    return data->line_limit;
}

//������� ��������� �����������
static CommentOption get_comments(LanguageOptions* options) {
    this = options;
    data = this->data;
    return data->comments;
}

// ������� ��������� ������� ����� GOSUB
static int get_gosub_limit(LanguageOptions* options) {
    this = options;
    data = this->data;
    return data->gosub_limit;
}

//����������� ������� ��������
static void destroy(LanguageOptions* options) {
    if (options) {
        if (options->data)
            free(options->data);
        free(options);
    }
}

//����������� ����� �����
LanguageOptions* new_LanguageOptions(void) {

    /* ��������� ������ */
    this = malloc(sizeof(LanguageOptions));
    data = this->data = malloc(sizeof(Private));

    /* ������������� ������� */
    this->set_line_numbers = set_line_numbers;
    this->set_line_limit = set_line_limit;
    this->set_comments = set_comments;
    this->set_gosub_limit = set_gosub_limit;
    this->get_line_numbers = get_line_numbers;
    this->get_line_limit = get_line_limit;
    this->get_comments = get_comments;
    this->get_gosub_limit = get_gosub_limit;
    this->destroy = destroy;

    /* ������������� ������� */
    data->line_numbers = LINE_NUMBERS_OPTIONAL;
    data->line_limit = 32767;
    data->comments = COMMENTS_ENABLED;
    data->gosub_limit = 64;

    /* ������� ������ ������� */
    return this;
}
