//������ ��������� ������.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"


/* ��������� ������. */
typedef struct {
  // ��������� ������������ ������.
  ErrorCode error;
  // �������� ������, � ������� ��������� ������.
  int line; 
  // ����� ��� �������� ������.
  int label;
} Private;


/* ��������������� ���������� */
// ������, ��� ������� ������� ������
ErrorHandler *this;
// ��������� ������ �������, ��� ������� ������� ������
Private *data; 

// ���� ������.
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



 // �������� ������������ ������.
static void set_code(ErrorHandler* errors, ErrorCode new_error, int new_line,
    int new_label) {

    // �������������
    this = errors;
    data = this->data;

    // ��������� �������
    data->error = new_error;
    data->line = new_line;
    data->label = new_label;
}

// ������� ����� ��������� ��������� ������.
static int get_label(ErrorHandler* errors) {
    this = errors;
    data = this->data;
    return data->label;
}

//������� ����� ��������� ��������� ������
static int get_line(ErrorHandler* errors) {
    this = errors;
    data = this->data;
    return data->line;
}

// ������� ��� ��������� ������������ ������.
static ErrorCode get_code(ErrorHandler* errors) {
    this = errors;
    data = this->data;
    return data->error;
}
//������������� ��������� �� ������.
static char* get_text(ErrorHandler* errors) {

    // ��������� ����������
    char
        * message, // ������ ���������
        * line_text, // ������ N
        * label_text; // ����� N

    // ������������� ������� ������
    this = errors;
    data = this->data;

    // ��������� �������� ������, ���� ��� ����
    line_text = malloc(20);
    if (data->line)
        sprintf(line_text, ", source line %d", data->line);
    else
        strcpy(line_text, "");

    // ��������� ����� ������, ���� ��� ����
    label_text = malloc(19);
    if (data->label)
        sprintf(label_text, ", line label %d", data->label);
    else
        strcpy(label_text, "");

    // ����������� ��������� �� ������
    message = malloc(strlen(messages[data->error]) + strlen(line_text)
        + strlen(label_text) + 1);
    strcpy(message, messages[data->error]);
    strcat(message, line_text);
    strcat(message, label_text);
    free(line_text);
    free(label_text);

    // ������� ���������� ��������� �� ������
    return message;
}


 // �������� �����������.
ErrorHandler* new_ErrorHandler(void) {

    // ��������� ������
    this = malloc(sizeof(ErrorHandler));
    this->data = data = malloc(sizeof(Private));

    // ������������� �������
    this->set_code = set_code;
    this->get_code = get_code;
    this->get_line = get_line;
    this->get_label = get_label;
    this->get_text = get_text;
    this->destroy = destroy;

    // ������������� �������
    data->error = E_NONE;
    data->line = 0;
    data->label = 0;

    // ������� ������ �������
    return this;
}

// ���������� ErrorHandler
static void destroy(ErrorHandler* errors) {
    if ((this = errors)) {
        data = this->data;
        free(data);
        free(this);
    }
}