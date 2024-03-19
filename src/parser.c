//Parser module

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "errors.h"
#include "options.h"
#include "token.h"
#include "tokeniser.h"
#include "parser.h"
#include "expression.h"

// Функция parse_expression() имеет прямую ссылку из функции parse_factor()
static ExpressionNode* parse_expression(void);

// Функция parse_statement() имеет прямую ссылку из функции parse_if_statement()
static StatementNode* parse_statement(void);

// Приватные данные 
typedef struct parser_data {
    int last_label; // последний метка строки, с которой столкнулись 
    int current_line; // последняя разобранная исходная строка 
    int end_of_file; // сигнал о конце файла 
    Token* stored_token; // заранее прочитанный токен 
    TokenStream* stream; // входной поток 
    ErrorHandler* errors; // обработчик ошибок парсинга 
    LanguageOptions* options; // параметры языка
} ParserData;

// Вспомогательные переменные
static Parser* this; // текущий парсер

// Приватные методы

// Получение следующий токен для разбора из буфера заранее прочитанных токенов или токенизатора.

static Token* get_token_to_parse() {
    Token* token; // токен для возврата

    // Получить токен одним способом или другим
    if (this->priv->stored_token) {
        token = this->priv->stored_token;
        this->priv->stored_token = NULL;
    }
    else
        token = this->priv->stream->next(this->priv->stream);

    // Сохранить номер строки, проверить EOF и вернуть токен
    this->priv->current_line = token->get_line(token);
    if (token->get_class(token) == TOKEN_EOF)
        this->priv->end_of_file = !0;
    return token;
}

// Разбор фактора
static FactorNode* parse_factor(void) {
    Token* token; // токен для чтения
    FactorNode* factor = NULL; // фактор, который мы построили
    ExpressionNode* expression = NULL; // выражение в скобках (если есть)
    int start_line; // строка, на которой находится этот фактор

    // Инициализация фактора и получение следующего токена
    factor = factor_create();
    token = get_token_to_parse();
    start_line = token->get_line(token);

    // Интерпретация знака
    if (token->get_class(token) == TOKEN_PLUS ||
        token->get_class(token) == TOKEN_MINUS) {
        factor->sign = (token->get_class(token) == TOKEN_PLUS) ? SIGN_POSITIVE : SIGN_NEGATIVE;
        token->destroy(token);
        token = get_token_to_parse();
    }

    // Интерпретация числа
    if (token->get_class(token) == TOKEN_NUMBER) {
        factor->class = FACTOR_VALUE;
        factor->data.value = atoi(token->get_content(token));
        if (factor->data.value < -32768 || factor->data.value > 32767)
            this->priv->errors->set_code(this->priv->errors, E_OVERFLOW, start_line, this->priv->last_label);
        token->destroy(token);
    }

    // Интерпретация переменной
    else if (token->get_class(token) == TOKEN_VARIABLE) {
        factor->class = FACTOR_VARIABLE;
        factor->data.variable = (int)(*token->get_content(token)) & 0x1F;
        token->destroy(token);
    }

    // Интерпретация выражения в скобках
    else if (token->get_class(token) == TOKEN_LEFT_PARENTHESIS) {

        // Разбор выражения в скобках и завершение фактора
        token->destroy(token);
        expression = parse_expression();
        if (expression) {
            token = get_token_to_parse();
            if (token->get_class(token) == TOKEN_RIGHT_PARENTHESIS) {
                factor->class = FACTOR_EXPRESSION;
                factor->data.expression = expression;
            }
            else {
                this->priv->errors->set_code(this->priv->errors, E_MISSING_RIGHT_PARENTHESIS, start_line, this->priv->last_label);
                factor_destroy(factor);
                factor = NULL;
                expression_destroy(expression);
            }
            token->destroy(token);
        }

        // Очистка после некорректного выражения в скобках
        else {
            this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, token->get_line(token), this->priv->last_label);
            token->destroy(token);
            factor_destroy(factor);
            factor = NULL;
        }
    }

    // Обработка других ошибок
    else {
        this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, token->get_line(token), this->priv->last_label);
        factor_destroy(factor);
        token->destroy(token);
        factor = NULL;
    }

    // Возврат фактора
    return factor;
}

// Разбор терма
static TermNode* parse_term(void) {

    TermNode* term = NULL; // терм, который мы строим
    FactorNode* factor = NULL; // обнаруженный фактор
    RightHandFactor* rhptr = NULL; // предыдущий правый фактор
    RightHandFactor* rhfactor = NULL; // обнаруженный правый фактор
    Token* token = NULL; // токен, прочитанный при поиске оператора

    // Сканируем первый фактор
    if ((factor = parse_factor())) {
        term = term_create();
        term->factor = factor;
        term->next = NULL;

        // Ищем последующие факторы
        while ((token = get_token_to_parse()) &&
            !this->priv->errors->get_code(this->priv->errors) &&
            (token->get_class(token) == TOKEN_MULTIPLY ||
                token->get_class(token) == TOKEN_DIVIDE)) {

            // Разбираем знак и фактор
            rhfactor = rhfactor_create();
            rhfactor->op = (token->get_class(token) == TOKEN_MULTIPLY) ? TERM_OPERATOR_MULTIPLY : TERM_OPERATOR_DIVIDE;
            if ((rhfactor->factor = parse_factor())) {
                rhfactor->next = NULL;
                if (rhptr)
                    rhptr->next = rhfactor;
                else
                    term->next = rhfactor;
                rhptr = rhfactor;
            }
            else {
                rhfactor_destroy(rhfactor);
                if (!this->priv->errors->get_code(this->priv->errors))
                    this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, token->get_line(token), this->priv->last_label);
            }

            // Очистка токена
            token->destroy(token);
        }

        // Мы прочитали за пределы терма; возвращаем токен
        this->priv->stored_token = token;
    }

    // Возвращаем разобранный терм, если есть
    return term;
}

// Разбор выражения
static ExpressionNode* parse_expression(void) {
    ExpressionNode* expression = NULL; // выражение, которое мы строим
    TermNode* term; // обнаруженный терм
    RightHandTerm* rhterm = NULL; // обнаруженный правый терм
    RightHandTerm* rhptr = NULL; // указатель на предыдущий правый терм
    Token* token; // токен, прочитанный при сканировании правых термов

    // Сканируем первый терм
    if ((term = parse_term())) {
        expression = expression_create();
        expression->term = term;
        expression->next = NULL;

        // Ищем последующие термы
        while ((token = get_token_to_parse()) &&
            !this->priv->errors->get_code(this->priv->errors) &&
            (token->get_class(token) == TOKEN_PLUS ||
                token->get_class(token) == TOKEN_MINUS ||
                token->get_class(token) == TOKEN_RND)) {

            // Разбираем знак и терм
            rhterm = rhterm_create();
            rhterm->op = (token->get_class(token) == TOKEN_PLUS) ? EXPRESSION_OPERATOR_PLUS :
                (token->get_class(token) == TOKEN_MINUS) ? EXPRESSION_OPERATOR_MINUS : EXPRESSION_OPERATOR_RND;
            if ((rhterm->term = parse_term())) {
                rhterm->next = NULL;
                if (rhptr)
                    rhptr->next = rhterm;
                else
                    expression->next = rhterm;
                rhptr = rhterm;
            }
            else {
                rhterm_destroy(rhterm);
                if (!this->priv->errors->get_code(this->priv->errors))
                    this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, token->get_line(token), this->priv->last_label);
            }

            // Очистка токена
            token->destroy(token);
        }

        // Мы прочитали за пределы терма; возвращаем токен
        this->priv->stored_token = token;
    }

    // Возвращаем разобранное выражение, если есть
    return expression;
}

/// Расчет числовой метки строки в соответствии с параметрами языка.
// Она будет использоваться, если строка не имеет указанной метки.
static int generate_default_label(void) {
    if (this->priv->options->get_line_numbers(this->priv->options) == LINE_NUMBERS_IMPLIED)
        return this->priv->last_label + 1;
    else
        return 0;
}

// Проверка валидности числовой метки строки в соответствии с параметрами языка
static int validate_line_label(int label) {

    // Числовая метка строки должна быть неотрицательной и в пределах заданного ограничения
    if (label < 0 || label > this->priv->options->get_line_limit(this->priv->options))
        return 0;

    // Числовая метка строки должна быть ненулевой, если она необязательна
    if (label == 0 && this->priv->options->get_line_numbers(this->priv->options) != LINE_NUMBERS_OPTIONAL)
        return 0;

    // Числовая метка строки должна быть возрастающей, если она необязательна
    if (label <= this->priv->last_label && this->priv->options->get_line_numbers(this->priv->options) != LINE_NUMBERS_OPTIONAL)
        return 0;

    // Если все проверки выше пройдены, числовая метка строки действительна
    return 1;
}

// Разбор оператора LET
static StatementNode* parse_let_statement(void) {
    Token* token; // токены, прочитанные как часть оператора LET
    int line; // строка, содержащая токен LET
    StatementNode* statement; // новый оператор

    // Инициализация оператора
    statement = statement_create();
    statement->class = STATEMENT_LET;
    statement->statement.letn = statement_create_let();
    line = this->priv->stream->get_line(this->priv->stream);

    // Определение переменной, которую мы присваиваем
    token = get_token_to_parse();
    if (token->get_class(token) != TOKEN_VARIABLE) {
        this->priv->errors->set_code(this->priv->errors, E_INVALID_VARIABLE, line, this->priv->last_label);
        statement_destroy(statement);
        token->destroy(token);
        return NULL;
    }
    statement->statement.letn->variable = *(token->get_content(token)) & 0x1f;

    // Получение "="
    token->destroy(token);
    token = get_token_to_parse();
    if (token->get_class(token) != TOKEN_EQUAL) {
        this->priv->errors->set_code(this->priv->errors, E_INVALID_ASSIGNMENT, line, this->priv->last_label);
        statement_destroy(statement);
        token->destroy(token);
        return NULL;
    }
    token->destroy(token);

    // Получение выражения
    statement->statement.letn->expression = parse_expression();
    if (!statement->statement.letn->expression) {
        this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, line, this->priv->last_label);
        statement_destroy(statement);
        return NULL;
    }

    // Возвращение оператора
    return statement;
}


// Разбор оператора IF
static StatementNode* parse_if_statement(void) {

    Token* token; // токены, прочитанные как часть оператора
    StatementNode* statement; // оператор IF

    // Инициализация оператора
    statement = statement_create();
    statement->class = STATEMENT_IF;
    statement->statement.ifn = statement_create_if();

    // Разбор первого выражения
    statement->statement.ifn->left = parse_expression();

    // Разбор оператора
    if (!this->priv->errors->get_code(this->priv->errors)) {
        token = get_token_to_parse();
        switch (token->get_class(token)) {
        case TOKEN_EQUAL:
            statement->statement.ifn->op = RELOP_EQUAL;
            break;
        case TOKEN_UNEQUAL:
            statement->statement.ifn->op = RELOP_UNEQUAL;
            break;
        case TOKEN_LESSTHAN:
            statement->statement.ifn->op = RELOP_LESSTHAN;
            break;
        case TOKEN_LESSOREQUAL:
            statement->statement.ifn->op = RELOP_LESSOREQUAL;
            break;
        case TOKEN_GREATERTHAN:
            statement->statement.ifn->op = RELOP_GREATERTHAN;
            break;
        case TOKEN_GREATEROREQUAL:
            statement->statement.ifn->op = RELOP_GREATEROREQUAL;
            break;
        default:
            this->priv->errors->set_code(this->priv->errors, E_INVALID_OPERATOR, token->get_line(token), this->priv->last_label);
        }
        token->destroy(token);
    }

    // Разбор второго выражения
    if (!this->priv->errors->get_code(this->priv->errors))
        statement->statement.ifn->right = parse_expression();

    // Разбор ключевого слова THEN
    if (!this->priv->errors->get_code(this->priv->errors)) {
        token = get_token_to_parse();
        if (token->get_class(token) != TOKEN_THEN)
            this->priv->errors->set_code(this->priv->errors, E_THEN_EXPECTED, token->get_line(token), this->priv->last_label);
        token->destroy(token);
    }

    // Разбор условного оператора
    if (!this->priv->errors->get_code(this->priv->errors))
        statement->statement.ifn->statement = parse_statement();

    // Уничтожение неполного оператора, если произошли ошибки
    if (this->priv->errors->get_code(this->priv->errors)) {
        statement_destroy(statement);
        statement = NULL;
    }

    // Возвращение оператора
    return statement;
}

// Разбор оператора GOTO
static StatementNode* parse_goto_statement(void) {
    StatementNode* statement; // оператор GOTO

    // Инициализация оператора
    statement = statement_create();
    statement->class = STATEMENT_GOTO;
    statement->statement.goton = statement_create_goto();

    // Разбор выражения с меткой строки
    if (!(statement->statement.goton->label = parse_expression())) {
        statement_destroy(statement);
        statement = NULL;
    }

    // Возвращение нового оператора
    return statement;
}

// Разбор оператора GOSUB
static StatementNode* parse_gosub_statement(void) {

    // Локальные переменные
    StatementNode* statement; // оператор GOSUB

    // Инициализация оператора
    statement = statement_create();
    statement->class = STATEMENT_GOSUB;
    statement->statement.gosubn = statement_create_gosub();

    // Разбор выражения с меткой строки
    if (!(statement->statement.gosubn->label = parse_expression())) {
        statement_destroy(statement);
        statement = NULL;
    }

    // Возвращение нового оператора
    return statement;
}

// Разбор оператора RETURN
static StatementNode* parse_return_statement(void) {
    StatementNode* statement; // оператор RETURN
    statement = statement_create();
    statement->class = STATEMENT_RETURN;
    return statement;
}

// Разбор оператора END
static StatementNode* parse_end_statement(void) {
    StatementNode* statement = NULL; // оператор END
    statement = statement_create();
    statement->class = STATEMENT_END;
    return statement;
}

// Разбор оператора PRINT
static StatementNode* parse_print_statement(void) {

    Token* token = NULL; // токены, прочитанные как часть оператора
    StatementNode* statement; // собранный оператор
    int line; // строка, содержащая токен PRINT
    OutputNode* nextoutput = NULL; // следующий узел вывода, который мы разбираем
    OutputNode* lastoutput = NULL; // последний разобранный узел вывода
    ExpressionNode* expression; // разобранное выражение

    // Инициализация оператора
    statement = statement_create();
    statement->class = STATEMENT_PRINT;
    statement->statement.printn = statement_create_print();
    line = this->priv->stream->get_line(this->priv->stream);

    // Главный цикл для разбора списка вывода
    do {

        // Удаление предыдущей запятой и чтение следующего значения вывода
        if (token)
            token->destroy(token);
        token = get_token_to_parse();

        // Обработка преждевременного конца строки
        if (token->get_class(token) == TOKEN_EOF || token->get_class(token) == TOKEN_EOL) {
            this->priv->errors->set_code(this->priv->errors, E_INVALID_PRINT_OUTPUT, line, this->priv->last_label);
            statement_destroy(statement);
            statement = NULL;
            token->destroy(token);
        }

        // Обработка литеральной строки
        else if (token->get_class(token) == TOKEN_STRING) {
            nextoutput = malloc(sizeof(OutputNode));
            nextoutput->class = OUTPUT_STRING;
            nextoutput->output.string = malloc(1 + strlen(token->get_content(token)));
            strcpy(nextoutput->output.string, token->get_content(token));
            nextoutput->next = NULL;
            token->destroy(token);
        }

        // Попытка обработать выражение
        else {
            this->priv->stored_token = token;
            if ((expression = parse_expression())) {
                nextoutput = malloc(sizeof(OutputNode));
                nextoutput->class = OUTPUT_EXPRESSION;
                nextoutput->output.expression = expression;
                nextoutput->next = NULL;
            }
            else {
                this->priv->errors->set_code(this->priv->errors, E_INVALID_PRINT_OUTPUT, token->get_line(token), this->priv->last_label);
                statement_destroy(statement);
                statement = NULL;
            }
        }

        // Добавление этого элемента вывода к оператору и поиск следующего
        if (!this->priv->errors->get_code(this->priv->errors)) {
            if (lastoutput)
                lastoutput->next = nextoutput;
            else
                statement->statement.printn->first = nextoutput;
            lastoutput = nextoutput;
            token = get_token_to_parse();
        }

        // Продолжение цикла, пока оператор не будет завершен
    } while (!this->priv->errors->get_code(this->priv->errors) &&
        (token->get_class(token) == TOKEN_COMMA || token->get_class(token) == TOKEN_SEMICOLON));

    // Вернуть последний токен и собранный оператор
    if (!this->priv->errors->get_code(this->priv->errors))
        this->priv->stored_token = token;
    return statement;
}
// Разбор оператора INPUT
static StatementNode* parse_input_statement(void) {

    Token* token = NULL; // токены, прочитанные как часть оператора
    StatementNode* statement; // собранный оператор
    int line; // строка, содержащая токен INPUT
    VariableListNode* nextvar = NULL; // следующий узел переменной, который мы разбираем
    VariableListNode* lastvar = NULL; // последний разобранный узел переменной

    // Инициализация оператора
    statement = statement_create();
    statement->class = STATEMENT_INPUT;
    statement->statement.inputn = statement_create_input();
    line = this->priv->stream->get_line(this->priv->stream);

    // Главный цикл для разбора списка переменных
    do {

        // Удаление предыдущей запятой и поиск следующей переменной
        if (token) token->destroy(token);
        token = get_token_to_parse();

        // Обработка преждевременного конца строки
        if (token->get_class(token) == TOKEN_EOF || token->get_line(token) != line) {
            this->priv->errors->set_code(this->priv->errors, E_INVALID_VARIABLE, line, this->priv->last_label);
            statement_destroy(statement);
            statement = NULL;
        }

        // Попытка обработки имени переменной
        else if (token->get_class(token) != TOKEN_VARIABLE) {
            this->priv->errors->set_code(this->priv->errors, E_INVALID_VARIABLE, token->get_line(token), this->priv->last_label);
            statement_destroy(statement);
            statement = NULL;
        }
        else {
            nextvar = malloc(sizeof(VariableListNode));
            nextvar->variable = *(token->get_content(token)) & 0x1f;
            nextvar->next = NULL;
            token->destroy(token);
        }

        // Добавление этой переменной в оператор и поиск следующей
        if (!this->priv->errors->get_code(this->priv->errors)) {
            if (lastvar)
                lastvar->next = nextvar;
            else
                statement->statement.inputn->first = nextvar;
            lastvar = nextvar;
            token = get_token_to_parse();
        }
    } while (!this->priv->errors->get_code(this->priv->errors) && token->get_class(token) == TOKEN_COMMA);

    // Возврат собранного оператора
    this->priv->stored_token = token;
    return statement;
}

//RND function call is an expression, and I'm dumb
// static StatementNode *parse_rnd_statement(void)
{

  // TODO implement
} 

// Разбор оператора из исходного файла
static StatementNode* parse_statement() {
    Token* token; // прочитанный токен
    StatementNode* statement = NULL; // новый оператор

    // Получение следующего токена
    token = get_token_to_parse();

    // Проверка команды
    switch (token->get_class(token)) {
    case TOKEN_EOL:
        this->priv->stored_token = token;
        statement = NULL;
        break;
    case TOKEN_LET:
        token->destroy(token);
        statement = parse_let_statement();
        break;
    case TOKEN_IF:
        token->destroy(token);
        statement = parse_if_statement();
        break;
    case TOKEN_GOTO:
        token->destroy(token);
        statement = parse_goto_statement();
        break;
    case TOKEN_GOSUB:
        token->destroy(token);
        statement = parse_gosub_statement();
        break;
    case TOKEN_RETURN:
        token->destroy(token);
        statement = parse_return_statement();
        break;
    case TOKEN_END:
        token->destroy(token);
        statement = parse_end_statement();
        break;
    case TOKEN_PRINT:
        token->destroy(token);
        statement = parse_print_statement();
        break;
    case TOKEN_INPUT:
        token->destroy(token);
        statement = parse_input_statement();
        break;
    default:
        this->priv->errors->set_code(this->priv->errors, E_UNRECOGNISED_COMMAND, token->get_line(token), this->priv->last_label);
        token->destroy(token);
    }

    // Возврат оператора
    return statement;
}
// Функция выполняет парсинг строки из исходного файла.
static ProgramLineNode* parse_program_line(void) {

    // Локальные переменные
    Token* token; // считанный токен
    ProgramLineNode* program_line; // считанная строка программы
    int label_encountered = 0; // флаг, равен 1, если в этой строке есть явная метка

    // Инициализируем строку программы и получаем первый токен
    program_line = program_line_create();
    program_line->label = generate_default_label();
    token = get_token_to_parse();

    // Обрабатываем конец файла
    if (token->get_class(token) == TOKEN_EOF) {
        token->destroy(token);
        program_line_destroy(program_line);
        return NULL;
    }

    // Обрабатываем метку строки, если есть
    if (token->get_class(token) == TOKEN_NUMBER) {
        program_line->label = atoi(token->get_content(token));
        label_encountered = 1;
        token->destroy(token);
    }
    else
        this->priv->stored_token = token;

    // Проверяем указанную или предполагаемую метку строки
    if (!validate_line_label(program_line->label)) {
        this->priv->errors->set_code
        (this->priv->errors, E_INVALID_LINE_NUMBER, this->priv->current_line,
            program_line->label);
        program_line_destroy(program_line);
        return NULL;
    }
    if (label_encountered)
        this->priv->last_label = program_line->label;

    // Проверяем наличие выражения и EOL
    program_line->statement = parse_statement();
    if (!this->priv->errors->get_code(this->priv->errors)) {
        token = get_token_to_parse();
        if (token->get_class(token) != TOKEN_EOL
            && token->get_class(token) != TOKEN_EOF)
            this->priv->errors->set_code
            (this->priv->errors, E_UNEXPECTED_PARAMETER, this->priv->current_line,
                this->priv->last_label);
        token->destroy(token);
    }
    if (program_line->statement)
        this->priv->last_label = program_line->label;

    // Возвращаем строку программы
    return program_line;
}


// Функция выполняет парсинг всей программы.
    // Принимает указатель на объект Parser.
    // Возвращает указатель на структуру ProgramNode, содержащую результаты парсинга.

static ProgramNode* parse(Parser* parser) {
    ProgramNode* program; // хранит собранную программу 
    ProgramLineNode
        * previous = NULL, // предыдущая строка 
        * current; // текущая строка 

    // инициализация программы
    this = parser; // Устанавливаем указатель "this" на объект Parser, чтобы использовать его приватные переменные
    program = malloc(sizeof(ProgramNode)); // Выделяем память для структуры ProgramNode
    program->first = NULL; // Инициализируем указатель на первую строку программы

    while ((current = parse_program_line()) // Читаем строки программы с помощью функции parse_program_line
        && !this->priv->errors->get_code(this->priv->errors)) { // Пока строка считывается успешно и нет ошибок
        if (previous)
            previous->next = current; // Если предыдущая строка существует, устанавливаем у нее указатель на следующую строку
        else
            program->first = current; // Иначе, устанавливаем указатель первой строки программы на текущую строку
        previous = current; // Присваиваем текущую строку переменной previous
    }

    return program; // Возвращаем собранную программу
}

// Функция возвращает текущую парсируемую строку исходного кода
static int get_line(Parser* parser) {
    return parser->priv->current_line; // Возвращаем номер текущей строки исходного кода
}

// Функция возвращает метку текущей парсируемой строки исходного кода.
static int get_label(Parser* parser) {
    return parser->priv->last_label; // Возвращаем метку текущей строки исходного кода
}

// Функция уничтожает объект парсера и освобождает выделенную память.
// Принимает указатель на объект Parser.

void destroy(Parser* parser) {
    if (parser) {
        if (parser->priv) {
            if (parser->priv->stream)
                parser->priv->stream->destroy(parser->priv->stream); // Уничтожаем объект stream, если он существует
        }
        free(parser->priv); // Освобождаем память, выделенную для приватных переменных парсера
    }
    free(parser); // Освобождаем память, выделенную для парсера
}

// Функция создает новый объект парсера.
Parser* new_Parser(ErrorHandler* errors, LanguageOptions* options,
    FILE* input) {

    // выделение памяти
    this = malloc(sizeof(Parser));
    this->priv = malloc(sizeof(ParserData));
    this->priv->stream = new_TokenStream(input); // Создание нового объекта TokenStream для работы с входным файлом

    // инициализация методов
    this->parse = parse; // Устанавливаем метод parse
    this->get_line = get_line; // Устанавливаем метод get_line
    this->get_label = get_label; // Устанавливаем метод get_label
    this->destroy = destroy; // Устанавливаем метод destroy

    // инициализация свойств
    this->priv->last_label = 0; // Инициализируем последнюю метку
    this->priv->current_line = 0; // Инициализируем текущую строку
    this->priv->end_of_file = 0; // Инициализируем флаг конца файла
    this->priv->stored_token = NULL; // Инициализируем хранимый токен
    this->priv->errors = errors; // Устанавливаем обработчик ошибок
    this->priv->options = options; // Устанавливаем опции языка

    // возвращаем новый объект
    return this;
}
