// Модуль вывода на C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "statement.h"
#include "expression.h"
#include "errors.h"
#include "parser.h"
#include "options.h"
#include "generatec.h"

// Список меток
typedef struct label {
    int number; // номер метки
    struct label* next; // следующая метка
} CLabel;

typedef struct {
    unsigned int input_used : 1; // true, если нам нужна процедура ввода
    unsigned long int vars_used : 26; // true для каждой используемой переменной
    CLabel* first_label; // начало списка меток
    char* code; // основной блок сгенерированного кода
    ErrorHandler* errors; // обработчик ошибок для компиляции
    LanguageOptions* options; // опции языка для компиляции
} Private;

static CProgram* this; // объект, над которым работаем
static Private* data; // приватные данные объекта
static ErrorHandler* errors; // обработчик ошибок
static LanguageOptions* options; // опции языка

// Прямые ссылки

// factor_output() имеет прямую ссылку на output_expression()
static char* output_expression(ExpressionNode* expression);

// output_statement() имеет прямую ссылку из output_if()
static char* output_statement(StatementNode* statement);

// Функции уровня 6

// Вывод фактора
static char* output_factor(FactorNode* factor) {

    // локальные переменные
    char* factor_text = NULL, // текст всего фактора
        * factor_buffer = NULL, // временный буфер для добавления к factor_text
        * expression_text = NULL; // текст подвыражения

    // вычисляем основной текст фактора
    switch (factor->class) {
    case FACTOR_VARIABLE:
        factor_text = malloc(2);
        sprintf(factor_text, "%c", factor->data.variable + 'a' - 1);
        data->vars_used |= 1 << (factor->data.variable - 1);
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
        errors->set_code(errors, E_INVALID_EXPRESSION, 0, 0);
    }
    // применяем отрицательный знак, если необходимо
    if (factor_text && factor->sign == SIGN_NEGATIVE) {
        factor_buffer = malloc(strlen(factor_text) + 2);
        sprintf(factor_buffer, "-%s", factor_text);
        free(factor_text);
        factor_text = factor_buffer;
    }
    // возвращаем окончательное представление фактора
    return factor_text;
}

// Функции уровня 5

// Вывод операторов умножения и деления
static char* output_term(TermNode* term) {
    // локальные переменные
    char
        * term_text = NULL, // текст всего терма
        * factor_text = NULL, // текст каждого фактора
        operator_char; // оператор, объединяющий правый фактор
    RightHandFactor* rhfactor; // правые факторы выражения
    // начинаем с начального фактора
    if ((term_text = output_factor(term->factor))) {
        rhfactor = term->next;
        while (!errors->get_code(errors) && rhfactor) {
            // определяем текст оператора
            switch (rhfactor->op) {
            case TERM_OPERATOR_MULTIPLY:
                operator_char = '*';
                break;
            case TERM_OPERATOR_DIVIDE:
                operator_char = '/';
                break;
            default:
                errors->set_code(errors, E_INVALID_EXPRESSION, 0, 0);
                free(term_text);
                term_text = NULL;
            }
            // получаем фактор, следующий за оператором
            if (!errors->get_code(errors)
                && (factor_text = output_factor(rhfactor->factor))) {
                term_text = realloc(term_text,
                    strlen(term_text) + strlen(factor_text) + 2);
                sprintf(term_text, "%s%c%s", term_text, operator_char, factor_text);
                free(factor_text);
            }

            // ищем еще один терм справа от выражения
            rhfactor = rhfactor->next;
        }
    }
    // возвращаем текст выражения
    return term_text;
}

// Функции уровня 4
// Вывод выражения для списка программы
static char* output_expression(ExpressionNode* expression) {
    // локальные переменные
    char
        * expression_text = NULL, // текст всего выражения
        * term_text = NULL, // текст каждого терма
        operator_char; // оператор, объединяющий правый терм
    RightHandTerm* rhterm; // правые термы выражения
    // начинаем с начального терма
    if ((expression_text = output_term(expression->term))) {
        rhterm = expression->next;
        while (!errors->get_code(errors) && rhterm) {

            // определяем текст оператора
            switch (rhterm->op) {
            case EXPRESSION_OPERATOR_PLUS:
                operator_char = '+';
                break;
            case EXPRESSION_OPERATOR_MINUS:
                operator_char = '-';
                break;
            default:
                errors->set_code(errors, E_INVALID_EXPRESSION, 0, 0);
                free(expression_text);
                expression_text = NULL;
            }
            // получаем термы, следующие за операторами
            if (!errors->get_code(errors)
                && (term_text = output_term(rhterm->term))) {
                expression_text = realloc(expression_text,
                    strlen(expression_text) + strlen(term_text) + 2);
                sprintf(expression_text, "%s%c%s", expression_text, operator_char,
                    term_text);
                free(term_text);
            }
            // ищем еще один терм справа от выражения
            rhterm = rhterm->next;
        }
    }
    // возвращаем текст выражения
    return expression_text;
}

// Функции уровня 3
// Вывод оператора LET
static char* output_let(LetStatementNode* letn) {
    // локальные переменные
    char
        * let_text = NULL, // текст оператора LET для сборки
        * expression_text = NULL; // текст выражения
    // сборка выражения
    expression_text = output_expression(letn->expression);

    // сборка окончательного текста LET, если у нас есть выражение
    if (expression_text) {
        let_text = malloc(4 + strlen(expression_text));
        sprintf(let_text, "%c=%s;", 'a' - 1 + letn->variable, expression_text);
        free(expression_text);
        data->vars_used |= 1 << (letn->variable - 1);
    }
    // возвращаем его
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
    case RELOP_EQUAL: strcpy(op_text, "=="); break;
    case RELOP_UNEQUAL: strcpy(op_text, "!="); break;
    case RELOP_LESSTHAN: strcpy(op_text, "<"); break;
    case RELOP_LESSOREQUAL: strcpy(op_text, "<="); break;
    case RELOP_GREATERTHAN: strcpy(op_text, ">"); break;
    case RELOP_GREATEROREQUAL: strcpy(op_text, ">="); break;
    }
    // сборка окончательного текста IF, если у нас есть все необходимое
    if (left_text && op_text && right_text && statement_text) {
        if_text = malloc(4 + strlen(left_text) + strlen(op_text) +
            strlen(right_text) + 3 + strlen(statement_text) + 2);
        sprintf(if_text, "if (%s%s%s) {%s}", left_text, op_text, right_text,
            statement_text);
    }
    // освобождаем временные ресурсы
    if (left_text) free(left_text);
    if (op_text) free(op_text);
    if (right_text) free(right_text);
    if (statement_text) free(statement_text);
    // возвращаем его
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
    // сборка окончательного текста LET, если у нас есть выражение
    if (expression_text) {
        goto_text = malloc(27 + strlen(expression_text));
        sprintf(goto_text, "label=%s; goto goto_block;", expression_text);
        free(expression_text);
    }
    // возвращаем его
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
    // сборка окончательного текста LET, если у нас есть выражение
    if (expression_text) {
        gosub_text = malloc(12 + strlen(expression_text));
        sprintf(gosub_text, "bas_exec(%s);", expression_text);
        free(expression_text);
    }
    // возвращаем его
    return gosub_text;
}

// Вывод оператора END
static char* output_end(void) {
    char* end_text; // полный текст команды END
    end_text = malloc(9);
    strcpy(end_text, "exit(0);");
    return end_text;
}

// Вывод оператора RETURN
static char* output_return(void) {
    char* return_text; // полный текст команды RETURN
    return_text = malloc(8);
    strcpy(return_text, "return;");
    return return_text;
}

// Вывод оператора PRINT
static char* output_print(PrintStatementNode* printn) {
    // локальные переменные
    char
        * format_text = NULL, // строка формата для printf
        * output_list = NULL, // список вывода для printf
        * output_text = NULL, // отдельный элемент вывода
        * print_text = NULL; // текст оператора PRINT для сборки
    OutputNode* output; // текущий элемент вывода
    // инициализация строк формата и вывода
    format_text = malloc(1);
    *format_text = '\0';
    output_list = malloc(1);
    *output_list = '\0';
    // сборка строки формата и вывода
    if ((output = printn->first)) {
        do {

            // форматирование элемента вывода
            switch (output->class) {
            case OUTPUT_STRING:
                format_text = realloc(format_text,
                    strlen(format_text) + strlen(output->output.string) + 1);
                strcat(format_text, output->output.string);
                break;
            case OUTPUT_EXPRESSION:
                format_text = realloc(format_text, strlen(format_text) + 3);
                strcat(format_text, "%d");
                output_text = output_expression(output->output.expression);
                output_list = realloc(output_list,
                    strlen(output_list) + 1 + strlen(output_text) + 1);
                strcat(output_list, ",");
                strcat(output_list, output_text);
                free(output_text);
                break;
            }
            // ищем следующий элемент вывода
        } while ((output = output->next));
    }
    // сборка всего текста оператора PRINT и его возвращение
    print_text = malloc(8 + strlen(format_text) + 3 + strlen(output_list) + 3);
    sprintf(print_text, "printf(\"%s\\n\"%s);", format_text, output_list);
    free(format_text);
    free(output_list);
    return print_text;
}

// Вывод оператора INPUT
static char* output_input(InputStatementNode* inputn) {
    // локальные переменные
    char
        * var_text, // текст ввода для одной переменной
        * input_text; // текст оператора INPUT для сборки
    VariableListNode* variable; // текущий элемент вывода

    // генерация строки ввода для каждой перечисленной переменной
    input_text = malloc(1);
    *input_text = '\0';
    if ((variable = inputn->first)) {
        do {
            var_text = malloc(18);
            sprintf(var_text, "%s%c = bas_input();",
                (variable == inputn->first) ? "" : "\n",
                variable->variable + 'a' - 1);
            input_text = realloc(input_text,
                strlen(input_text) + strlen(var_text) + 1);
            strcat(input_text, var_text);
            free(var_text);
            data->vars_used |= 1 << (variable->variable - 1);
        } while ((variable = variable->next));
    }
    data->input_used = 1;
    // возвращение собранного текста
    return input_text;
}


//Функции уровня 2


 // Вывод оператора
static char* output_statement(StatementNode* statement) {
    // локальные переменные
    char* output = NULL; // выводимый текст
    // возвращаем пустой вывод для комментариев
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
        strcpy(output, "Нераспознанный оператор.");
    }
    // возвращаем строку с оператором
    return
}


// Функции уровня 1

 // Генерация строки программы
static void generate_line(ProgramLineNode* program_line) {
    // локальные переменные
    CLabel
        * prior_label, // метка перед потенциальной точкой вставки
        * next_label, // метка после потенциальной точки вставки
        * new_label; // метка для вставки
    char
        label_text[12], // текст метки строки
        * statement_text; // текст оператора

    // генерация метки строки
    if (program_line->label) {
        // вставка метки в список меток
        new_label = malloc(sizeof(CLabel));
        new_label->number = program_line->label;
        new_label->next = NULL;
        prior_label = NULL;
        next_label = data->first_label;
        while (next_label && next_label->number < new_label->number) {
            prior_label = next_label;
            next_label = prior_label->next;
        }
        new_label->next = next_label;
        if (prior_label)
            prior_label->next = new_label;
        else
            data->first_label = new_label;
        // добавление метки в блок кода
        sprintf(label_text, "lbl_%d:\n", program_line->label);
        data->code = realloc(data->code,
            strlen(data->code) + strlen(label_text) + 1);
        strcat(data->code, label_text);
    }
    // генерация оператора и добавление его, если это не комментарий
    statement_text = output_statement(program_line->statement);
    if (statement_text) {
        data->code = realloc(data->code,
            strlen(data->code) + strlen(statement_text) + 2);
        strcat(data->code, statement_text);
        strcat(data->code, "\n");
        free(statement_text);
    }
}


// Генерация строк #include и #define
static void generate_includes(void) {
    // локальные переменные
    char
        include_text[1024], // полный текст #include и #define
        define_text[80]; // отдельная строка #define

    // сборка #include и #define
    strcpy(include_text, "#include <stdio.h>\n");
    strcat(include_text, "#include <stdlib.h>\n");
    sprintf(define_text, "#define E_RETURN_WITHOUT_GOSUB %d\n",
        E_RETURN_WITHOUT_GOSUB);
    strcat(include_text, define_text);

    // добавление #include и #define в вывод
    this->c_output = realloc(this->c_output, strlen(this->c_output)
        + strlen(include_text) + 1);
    strcat(this->c_output, include_text);
}

// Генерация объявлений переменных
static void generate_variables(void) {
    // локальные переменные
    int vcount; // счетчик переменных
    char
        var_text[12], // текст отдельной переменной
        declaration[60]; // текст объявления

    // сбор объявления
    *declaration = '\0';
    for (vcount = 0; vcount < 26; ++vcount) {
        if (data->vars_used & 1 << vcount) {
            if (*declaration)
                sprintf(var_text, ",%c", 'a' + vcount);
            else
                sprintf(var_text, "short int %c", 'a' + vcount);
            strcat(declaration, var_text);
        }
    }

    // если есть переменные, добавляем объявление в вывод
    if (*declaration) {
        strcat(declaration, ";\n");
        this->c_output = realloc(this->c_output, strlen(this->c_output)
            + strlen(declaration) + 1);
        strcat(this->c_output, declaration);
    }
}

// Генерация функции bas_input
static void generate_bas_input(void) {

    // локальные переменные
    char function_text[1024]; // вся функция

    // конструирование текста функции
    strcpy(function_text, "short int bas_input (void) {\n");
    strcat(function_text, "short int ch, sign, value;\n");
    strcat(function_text, "do {\n");
    strcat(function_text, "if (ch == '-') sign = -1; else sign = 1;\n");
    strcat(function_text, "ch = getchar ();\n");
    strcat(function_text, "} while (ch < '0' || ch > '9');\n");
    strcat(function_text, "value = 0;\n");
    strcat(function_text, "do {\n");
    strcat(function_text, "value = 10 * value + (ch - '0');\n");
    strcat(function_text, "ch = getchar ();\n");
    strcat(function_text, "} while (ch >= '0' && ch <= '9');\n");
    strcat(function_text, "return sign * value;\n");
    strcat(function_text, "}\n");

    // добавление текста функции в вывод
    this->c_output = realloc(this->c_output, strlen(this->c_output)
        + strlen(function_text) + 1);
    strcat(this->c_output, function_text);
}

// Генерация функции bas_exec
static void generate_bas_exec(void) {

    // Локальные переменные
    char
        * op, // оператор сравнения для номеров строк
        goto_line[80], // строка в блоке goto
        * goto_block, // блок goto
        * function_text; // полный текст функции
    CLabel* label; // указатель на метку для построения блока goto

    // Определение, какой оператор использовать для сравнения
    op = (options->get_line_numbers(options) == LINE_NUMBERS_OPTIONAL) ? "==" : "<=";

    // Создание блока goto
    goto_block = malloc(128);
    strcpy(goto_block, "goto_block:\n");
    strcat(goto_block, "if (!label) goto lbl_start;\n");
    label = data->first_label;
    while (label) {
        sprintf(goto_line, "if (label%s%d) goto lbl_%d;\n", op, label->number, label->number);
        goto_block = realloc(goto_block, strlen(goto_block) + strlen(goto_line) + 1);
        strcat(goto_block, goto_line);
        label = label->next;
    }
    goto_block = realloc(goto_block, strlen(goto_block) + 12);
    strcat(goto_block, "lbl_start:\n");

    // Составление функции
    function_text = malloc(28 + strlen(goto_block) + strlen(data->code) + 3);
    strcpy(function_text, "void bas_exec(int label) {\n");
    strcat(function_text, goto_block);
    strcat(function_text, data->code);
    strcat(function_text, "}\n");

    // Добавление текста функции в вывод
    this->c_output = realloc(this->c_output, strlen(this->c_output) + strlen(function_text) + 1);
    strcat(this->c_output, function_text);
    free(goto_block);
    free(function_text);
}

// Генерация главной функции
void generate_main(void) {

    // Локальные переменные
    char function_text[1024]; // весь текст функции

    // Формирование текста функции
    strcpy(function_text, "int main(void) {\n");
    strcat(function_text, "bas_exec(0);\n");
    strcat(function_text, "exit(E_RETURN_WITHOUT_GOSUB);\n");
    strcat(function_text, "}\n");

    // Добавление текста функции в вывод
    this->c_output = realloc(this->c_output, strlen(this->c_output) + strlen(function_text) + 1);
    strcat(this->c_output, function_text);
}

// Функция генерации программы
static void generate(CProgram* c_program, ProgramNode* program) {
    // Локальные переменные
    ProgramLineNode* program_line; // строка для обработки
    // Инициализация этого объекта
    this = c_program;
    data = (Private*)c_program->private_data;
    // Генерация кода для строк
    program_line = program->first;
    while (program_line) {
        generate_line(program_line);
        program_line = program_line->next;
    }
    // Сборка кода
    generate_includes();
    generate_variables();
    if (data->input_used)
        generate_bas_input();
    generate_bas_exec();
    generate_main();
}

// Деструктор
static void destroy(CProgram* c_program) {
    // Локальные переменные
    CLabel
        * current_label, // указатель на метку для уничтожения
        * next_label; // указатель на следующую метку для уничтожения
    // Уничтожение приватных данных
    this = c_program;
    if (this->private_data) {
        data = (Private*)c_program->private_data;
        next_label = data->first_label;
        while (next_label) {
            current_label = next_label;
            next_label = current_label->next;
            free(current_label);
        }
        if (data->code)
            free(data->code);
        free(data);
    }
    // Уничтожение сгенерированного вывода
    if (this->c_output)
        free(this->c_output);
    // Уничтожение содержащей структуры
    free(c_program);
}

// Конструктор
CProgram* new_CProgram(ErrorHandler* compiler_errors, LanguageOptions* compiler_options) {

    // Выделение памяти
    this = malloc(sizeof(CProgram));
    this->private_data = data = malloc(sizeof(Private));
    // Инициализация методов
    this->generate = generate;
    this->destroy = destroy;
    // Инициализация свойств
    errors = data->errors = compiler_errors;
    options = data->options = compiler_options;
    data->input_used = 0;
    data->vars_used = 0;
    data->first_label = NULL;
    data->code = malloc(1);
    *data->code = '\0';
    this->c_output = malloc(1);
    *this->c_output = '\0';
    // Возврат созданной структуры
    return this;
}

