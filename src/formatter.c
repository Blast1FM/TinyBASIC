//  Модуль вывода листинга

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "formatter.h"
#include "expression.h"
#include "errors.h"
#include "parser.h"

// Приватные данные форматтера
typedef struct formatter_data {
    ErrorHandler* errors; // обработчик ошибок
} FormatterData;

// Удобные переменные
static Formatter* this; // объект, над которым работаем


// Прямые ссылки

// factor_output() имеет прямую ссылку на output_expression()
static char* output_expression(ExpressionNode* expression);

// output_statement() имеет прямую ссылку из output_if()
static char* output_statement(StatementNode* statement);

// Функции 


// Вывод выражения для сложения + вычитания.
static char* output_expression(ExpressionNode* expression) {
    // локальные переменные
    char
        * expression_text = NULL, // текст всего выражения
        * term_text = NULL, // текст каждого члена
        operator_char; // оператор, соединяющий правый член
    RightHandTerm* rhterm; // правые члены выражения
    // начало с начального члена
    if ((expression_text = output_term(expression->term))) {
        rhterm = expression->next;
        while (!this->priv->errors->get_code(this->priv->errors) && rhterm) {

            // определение текста оператора
            switch (rhterm->op) {
            case EXPRESSION_OPERATOR_PLUS:
                operator_char = '+';
                break;
            case EXPRESSION_OPERATOR_MINUS:
                operator_char = '-';
                break;
            default:
                this->priv->errors->set_code
                (this->priv->errors, E_INVALID_EXPRESSION, 0, 0);
                free(expression_text);
                expression_text = NULL;
            }
            // получение членов, следующих за операторами
            if (!this->priv->errors->get_code(this->priv->errors)
                && (term_text = output_term(rhterm->term))) {
                expression_text = realloc(expression_text,
                    strlen(expression_text) + strlen(term_text) + 2);
                sprintf(expression_text, "%s%c%s", expression_text, operator_char,
                    term_text);
                free(term_text);
            }
            // поиск еще одного члена справа от выражения
            rhterm = rhterm->next;
        }
    }
    // возврат текста выражения
    return expression_text;
} 
// Вывод члена для операций умножения и деления
static char* output_term(TermNode* term) {
    // локальные переменные
    char
        * term_text = NULL, // текст всего члена
        * factor_text = NULL, // текст каждого множителя
        operator_char; // оператор, соединяющий правую часть члена
    RightHandFactor* rhfactor; // правые множители выражения
    // начало с начального множителя
    if ((term_text = output_factor(term->factor))) {
        rhfactor = term->next;
        while (!this->priv->errors->get_code(this->priv->errors) && rhfactor) {

            // определение текста оператора
            switch (rhfactor->op) {
            case TERM_OPERATOR_MULTIPLY:
                operator_char = '*';
                break;
            case TERM_OPERATOR_DIVIDE:
                operator_char = '/';
                break;
            default:
                this->priv->errors->set_code
                (this->priv->errors, E_INVALID_EXPRESSION, 0, 0);
                free(term_text);
                term_text = NULL;
            }
            // получение множителя, следующего за оператором
            if (!this->priv->errors->get_code(this->priv->errors)
                && (factor_text = output_factor(rhfactor->factor))) {
                term_text = realloc(term_text,
                    strlen(term_text) + strlen(factor_text) + 2);
                sprintf(term_text, "%s%c%s", term_text, operator_char, factor_text);
                free(factor_text);
            }
            // поиск еще одного члена справа от выражения
            rhfactor = rhfactor->next;
        }
    }
    // возврат текста выражения
    return term_text;
}


// Вывод множителя
static char* output_factor(FactorNode* factor) {

    // локальные переменные
    char* factor_text = NULL, // текст всего множителя
        * factor_buffer = NULL, // временный буфер для добавления к factor_text
        * expression_text = NULL; // текст подвыражения
    // вычисление основного текста множителя
    switch (factor->class) {
    case FACTOR_VARIABLE:
        factor_text = malloc(2);
        sprintf(factor_text, "%c", factor->data.variable + 'A' - 1);
        break;
    case FACTOR_VALUE:
        factor_text = malloc(7);
        sprintf(factor_text, "%d", factor->data.value);
        break;
    case FACTOR_EXPRESSION:
        if ((expression_text = output_expression(factor->data.expression))) {
            factor_text = malloc(strlen(expression_text) + 3);
            sprintf(factor_text, "(%s)", expression_text);
            free(expression_text);
        }
        break;
    default:
        this->priv->errors->set_code
        (this->priv->errors, E_INVALID_EXPRESSION, 0, 0);
    }
    // применение отрицательного знака, если необходимо
    if (factor_text && factor->sign == SIGN_NEGATIVE) {
        factor_buffer = malloc(strlen(factor_text) + 2);
        sprintf(factor_buffer, "-%s", factor_text);
        free(factor_text);
        factor_text = factor_buffer;
    }
    // возврат окончательного представления множителя
    return factor_text;
}



// Вывод оператора LET
static char* output_let(LetStatementNode* letn) {
    // локальные переменные
    char
        * let_text = NULL, // текст оператора LET для сборки
        * expression_text = NULL; // текст выражения

    // сборка выражения
    expression_text = output_expression(letn->expression);
    // сборка окончательного текста оператора LET, если есть выражение
    if (expression_text) {
        let_text = malloc(7 + strlen(expression_text));
        sprintf(let_text, "LET %c=%s", 'A' - 1 + letn->variable, expression_text);
        free(expression_text);
    }
    // возврат
    return let_text;
}

// Вывод оператора IF
static char* output_if(IfStatementNode* ifn) {
    // локальные переменные
    char
        * if_text = NULL, // текст оператора IF для сборки
        * left_text = NULL, // текст левого выражения
        * op_text = NULL, // текст оператора
        * right_text = NULL, // текст правого выражения
        * statement_text = NULL; // текст условного оператора

    // сборка выражений и условного оператора
    left_text = output_expression(ifn->left);
    right_text = output_expression(ifn->right);
    statement_text = output_statement(ifn->statement);
    // определение текста оператора
    op_text = malloc(3);
    switch (ifn->op) {
    case RELOP_EQUAL: strcpy(op_text, "="); break;
    case RELOP_UNEQUAL: strcpy(op_text, "<>"); break;
    case RELOP_LESSTHAN: strcpy(op_text, "<"); break;
    case RELOP_LESSOREQUAL: strcpy(op_text, "<="); break;
    case RELOP_GREATERTHAN: strcpy(op_text, ">"); break;
    case RELOP_GREATEROREQUAL: strcpy(op_text, ">="); break;
    }
    // сборка окончательного текста оператора IF, если есть все необходимое
    if (left_text && op_text && right_text && statement_text) {
        if_text = malloc(3 + strlen(left_text) + strlen(op_text) +
            strlen(right_text) + 6 + strlen(statement_text) + 1);
        sprintf(if_text, "IF %s%s%s THEN %s", left_text, op_text, right_text,
            statement_text);
    }
    // освобождение временных ресурсов
    if (left_text) free(left_text);
    if (op_text) free(op_text);
    if (right_text) free(right_text);
    if (statement_text) free(statement_text);
    // возврат
    return if_text;
}

// Вывод оператора GOTO
static char* output_goto(GotoStatementNode* goton) {
    // локальные переменные
    char
        * goto_text = NULL, // текст оператора GOTO для сборки
        * expression_text = NULL; // текст выражения
    // сборка выражения
    expression_text = output_expression(goton->label);
    // сборка окончательного текста оператора GOTO, если есть выражение
    if (expression_text) {
        goto_text = malloc(6 + strlen(expression_text));
        sprintf(goto_text, "GOTO %s", expression_text);
        free(expression_text);
    }
    // возврат
    return goto_text;
}


// Вывод оператора GOSUB
static char* output_gosub(GosubStatementNode* gosubn) {
    // локальные переменные
    char
        * gosub_text = NULL, // текст оператора GOSUB для сборки
        * expression_text = NULL; // текст выражения
    // сборка выражения
    expression_text = output_expression(gosubn->label);
    // сборка окончательного текста оператора GOSUB, если есть выражение
    if (expression_text) {
        gosub_text = malloc(7 + strlen(expression_text));
        sprintf(gosub_text, "GOSUB %s", expression_text);
        free(expression_text);
    }
    // возврат
    return gosub_text;
}


// Вывод оператора END
static char* output_end(void) {
    char* end_text; // полный текст команды END
    end_text = malloc(4);
    strcpy(end_text, "END");
    return end_text;
}


// Вывод оператора RETURN
static char* output_return(void) {
    char* return_text; // полный текст команды RETURN
    return_text = malloc(7);
    strcpy(return_text, "RETURN");
    return return_text;
}


// Вывод оператора PRINT
static char* output_print(PrintStatementNode* printn) {
    // локальные переменные
    char
        * print_text, // текст оператора PRINT для сборки
        * output_text = NULL; // текст текущего элемента вывода
    OutputNode* output; // текущий элемент вывода
    // инициализация оператора PRINT
    print_text = malloc(6);
    strcpy(print_text, "PRINT");
    // добавление элементов вывода
    if ((output = printn->first)) {
        do {

            // добавление разделителя
            print_text = realloc(print_text, strlen(print_text) + 2);
            strcat(print_text, output == printn->first ? " " : ",");

            // форматирование элемента вывода
            switch (output->class) {
            case OUTPUT_STRING:
                output_text = malloc(strlen(output->output.string) + 3);
                sprintf(output_text, "%c%s%c", '"', output->output.string, '"');
                break;
            case OUTPUT_EXPRESSION:
                output_text = output_expression(output->output.expression);
                break;
            }

            // добавление элемента вывода
            print_text = realloc(print_text,
                strlen(print_text) + strlen(output_text) + 1);
            strcat(print_text, output_text);
            free(output_text);

            // поиск следующего элемента вывода
        } while ((output = output->next));
    }
    // возврат собранного текста
    return print_text;
}

// Вывод оператора INPUT
static char* output_input(InputStatementNode* inputn) {
    // локальные переменные
    char
        * input_text, // текст оператора INPUT для сборки
        var_text[3]; // текстовое представление каждой переменной с разделителем
    VariableListNode* variable; // текущий элемент вывода
    // инициализация оператора INPUT
    input_text = malloc(6);
    strcpy(input_text, "INPUT");
    // добавление элементов вывода
    if ((variable = inputn->first)) {
        do {
            sprintf(var_text, "%c%c",
                (variable == inputn->first) ? ' ' : ',',
                variable->variable + 'A' - 1);
            input_text = realloc(input_text, strlen(input_text) + 3);
            strcat(input_text, var_text);
        } while ((variable = variable->next));
    }
    // возврат собранного текста
    return input_text;
}

// Вывод оператора
static char* output_statement(StatementNode* statement) {
    // локальные переменные
    char* output = NULL; // текст вывода
    // возвращение пустого вывода для комментариев
    if (!statement)
        return NULL;
    // сборка самого оператора
    switch (statement->class) {
    case STATEMENT_LET:
        output = output_let(statement->statement.letn);
        break;
    case STATEMENT_IF:
        output = output_if(statement->statement.ifn);
        break;
    case STATEMENT_GOTO:
        output = output_goto(statement->statement.goton);
        break;
    case STATEMENT_GOSUB:
        output = output_gosub(statement->statement.gosubn);
        break;
    case STATEMENT_RETURN:
        output = output_return();
        break;
    case STATEMENT_END:
        output = output_end();
        break;
    case STATEMENT_PRINT:
        output = output_print(statement->statement.printn);
        break;
    case STATEMENT_INPUT:
        output = output_input(statement->statement.inputn);
        break;
    default:
        output = malloc(24);
        strcpy(output, "Неизвестный оператор.");
    }

    // возврат строки списка
    return output;
}

// Вывод строки программы
static void generate_line(ProgramLineNode* program_line) {

    // локальные переменные
    char
        label_text[7], // текст метки строки
        * output = NULL, // остальной вывод
        * line_text = NULL; // собранная строка

    // инициализация метки строки
    if (program_line->label)
        sprintf(label_text, "%5d ", program_line->label);
    else
        strcpy(label_text, "      ");

    // сборка самого оператора
    output = output_statement(program_line->statement);

    // если это не комментарий, добавляем его в программу
    if (output) {
        line_text = malloc(strlen(label_text) + strlen(output) + 2);
        sprintf(line_text, "%s%s\n", label_text, output);
        free(output);
        this->output = realloc(this->output,
            strlen(this->output) + strlen(line_text) + 1);
        strcat(this->output, line_text);
        free(line_text);
    }
}


// Публичные методы
static void generate(Formatter* formatter, ProgramNode* program) {
    // локальные переменные
    ProgramLineNode* program_line; // строка для обработки
    // инициализация этого объекта
    this = formatter;
    // генерация кода для строк
    program_line = program->first;
    while (program_line) {
        generate_line(program_line);
        program_line = program_line->next;
    }
}

// Уничтожение форматтера при необходимости
static void destroy(Formatter* formatter) {
    if (formatter) {
        if (formatter->output)
            free(formatter->output);
        if (formatter->priv)
            free(formatter->priv);
        free(formatter);
    }
}

// Конструктор Formatter
Formatter* new_Formatter(ErrorHandler* errors) {

    // выделение памяти
    this = malloc(sizeof(Formatter));
    this->priv = malloc(sizeof(FormatterData));

    // инициализация методов
    this->generate = generate;
    this->destroy = destroy;

    // инициализация свойств
    this->output = malloc(sizeof(char));
    *this->output = '\0';
    this->priv->errors = errors;

    // возврат нового объекта
    return this;
}