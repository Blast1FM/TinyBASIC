// Модуль интерпретатора

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpret.h"
#include "errors.h"
#include "options.h"
#include "statement.h"

static int interpret_expression(ExpressionNode* expression);
static void interpret_statement(StatementNode* statement);

 // Стек GOSUB
typedef struct gosub_stack_node GosubStackNode;
typedef struct gosub_stack_node {
    ProgramLineNode* program_line; // строка после GOSUB
    GosubStackNode* next; // узел стека для предыдущего GOSUB
} GosubStackNode;

typedef struct interpreter_data {
    ProgramNode* program; // программа для интерпретации
    ProgramLineNode* line; // текущая выполняемая строка
    GosubStackNode* gosub_stack; // вершина стека GOSUB
    int gosub_stack_size; // количество записей в стеке GOSUB
    int variables[26]; // числовые переменные
    int stopped; // установлено в 1 при обнаружении END
    ErrorHandler* errors; // обработчик ошибок
    LanguageOptions* options; // опции языка
} InterpreterData;

static Interpreter* this; // объект, с которым мы работаем

 // Оценка множителя для интерпретатора
static int interpret_factor(FactorNode* factor) {
    // Локальные переменные
    int result_store = 0; // результат оценки множителя
    // Проверка класса множителя
    switch (factor->class) {
        // Обычная переменная
    case FACTOR_VARIABLE:
        result_store = this->priv->variables[factor->data.variable - 1] *
            (factor->sign == SIGN_POSITIVE ? 1 : -1);
        break;
        // Целочисленная константа
    case FACTOR_VALUE:
        result_store = factor->data.value *
            (factor->sign == SIGN_POSITIVE ? 1 : -1);
        break;
        // Выражение
    case FACTOR_EXPRESSION:
        result_store = interpret_expression(factor->data.expression) *
            (factor->sign == SIGN_POSITIVE ? 1 : -1);
        break;
        // Это происходит только в случае невыполнения обязанностей парсера
    default:
        this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, 0,
            this->priv->line->label);
    }
    // Проверяем результат и возвращаем его
    if (result_store < -32768 || result_store > 32767)
        this->priv->errors->set_code(this->priv->errors, E_OVERFLOW, 0,
            this->priv->line->label);
    return result_store;
}

// Оценка множителя
static int interpret_term(TermNode* term) {
    // Локальные переменные
    int result_store; // частичное вычисление
    RightHandFactor* rhfactor; // указатель на последующие узлы правой части
    int divisor; // используется для проверки деления на 0 перед попыткой
    // Вычисляем результат первого множителя
    result_store = interpret_factor(term->factor);
    rhfactor = term->next;
    // Корректируем результат в соответствии с последовательными множителями
    while (rhfactor && !this->priv->errors->get_code(this->priv->errors)) {
        switch (rhfactor->op) {
        case TERM_OPERATOR_MULTIPLY:
            result_store *= interpret_factor(rhfactor->factor);
            if (result_store < -32768 || result_store > 32767)
                this->priv->errors->set_code(this->priv->errors, E_OVERFLOW, 0,
                    this->priv->line->label);
            break;
        case TERM_OPERATOR_DIVIDE:
            if ((divisor = interpret_factor(rhfactor->factor)))
                result_store /= divisor;
            else
                this->priv->errors->set_code(this->priv->errors, E_DIVIDE_BY_ZERO, 0,
                    this->priv->line->label);
            break;
        default:
            break;
        }
        rhfactor = rhfactor->next;
    }
    // Возвращаем результат
    return result_store;
}

// Оценка выражения для интерпретатора
static int interpret_expression(ExpressionNode* expression) {

    // Локальные переменные
    int result_store; // частичное вычисление
    RightHandTerm* rhterm; // указатель на последующие узлы правой части

    // Вычисляем результат первого члена
    result_store = interpret_term(expression->term);
    rhterm = expression->next;

    // Корректируем результат в соответствии с последовательными членами
    while (rhterm && !this->priv->errors->get_code(this->priv->errors)) {
        switch (rhterm->op) {
        case EXPRESSION_OPERATOR_PLUS:
            result_store += interpret_term(rhterm->term);
            if (result_store < -32768 || result_store > 32767)
                this->priv->errors->set_code(this->priv->errors, E_OVERFLOW, 0,
                    this->priv->line->label);
            break;
        case EXPRESSION_OPERATOR_MINUS:
            result_store -= interpret_term(rhterm->term);
            if (result_store < -32768 || result_store > 32767)
                this->priv->errors->set_code(this->priv->errors, E_OVERFLOW, 0,
                    this->priv->line->label);
            break;
        default:
            break;
        }
        rhterm = rhterm->next;
    }
    // Возвращаем результат
    return result_store;
}

// Поиск строки программы по ее метке
static ProgramLineNode* find_label(int jump_label) {
    // Локальные переменные
    ProgramLineNode* ptr, * found = NULL; // строка, которую мы рассматриваем
    // Поиск
    for (ptr = this->priv->program->first; ptr && !found; ptr = ptr->next)
        if (ptr->label == jump_label)
            found = ptr;
        else if (ptr->label >= jump_label &&
            this->priv->options->get_line_numbers(this->priv->options) !=
            LINE_NUMBERS_OPTIONAL)
            found = ptr;
    // Проверяем на ошибки и возвращаем найденное
    if (!found)
        this->priv->errors->set_code(this->priv->errors, E_INVALID_LINE_NUMBER, 0,
            this->priv->line->label);
    return found;
}

//Уровень 1 Рутины

 // Инициализация переменных
static void initialise_variables(void) {
    int count; // счетчик для переменных
    for (count = 0; count < 26; ++count) {
        this->priv->variables[count] = 0;
    }
}

// Интерпретация оператора LET
void interpret_let_statement(LetStatementNode* letn) {
    this->priv->variables[letn->variable - 1] =
        interpret_expression(letn->expression);
    this->priv->line = this->priv->line->next;
}

// Интерпретация оператора IF
void interpret_if_statement(IfStatementNode* ifn) {
    // Локальные переменные
    int left, right, comparison; // результаты выражений
    // Получаем выражения
    left = interpret_expression(ifn->left);
    right = interpret_expression(ifn->right);
    // Делаем сравнение
    switch (ifn->op) {
    case RELOP_EQUAL:
        comparison = (left == right);
        break;
    case RELOP_UNEQUAL:
        comparison = (left != right);
        break;
    case RELOP_LESSTHAN:
        comparison = (left < right);
        break;
    case RELOP_LESSOREQUAL:
        comparison = (left <= right);
        break;
    case RELOP_GREATERTHAN:
        comparison = (left > right);
        break;
    case RELOP_GREATEROREQUAL:
        comparison = (left >= right);
        break;
    }
    // Выполняем условный оператор
    if (comparison && !this->priv->errors->get_code(this->priv->errors))
        interpret_statement(ifn->statement);
    else
        this->priv->line = this->priv->line->next;
}


// Интерпретация оператора GOTO
void interpret_goto_statement(GotoStatementNode* goton) {
    int label; // метка строки, к которой перейти
    label = interpret_expression(goton->label);
    if (!this->priv->errors->get_code(this->priv->errors))
        this->priv->line = find_label(label);
}

// Интерпретация оператора GOSUB
void interpret_gosub_statement(GosubStatementNode* gosubn) {
    // Локальные переменные
    GosubStackNode* gosub_node; // указывает на строку программы, к которой нужно вернуться
    int label; // метка строки, к которой перейти
    // Создаем новый узел на стеке GOSUB
    if (this->priv->gosub_stack_size < this->priv->options->get_gosub_limit
    (this->priv->options)) {
        gosub_node = malloc(sizeof(GosubStackNode));
        gosub_node->program_line = this->priv->line->next;
        gosub_node->next = this->priv->gosub_stack;
        this->priv->gosub_stack = gosub_node;
        ++this->priv->gosub_stack_size;
    }
    else
        this->priv->errors->set_code(this->priv->errors,
            E_TOO_MANY_GOSUBS, 0, this->priv->line->label);

    // Переходим к запрошенной подпрограмме
    if (!this->priv->errors->get_code(this->priv->errors))
        label = interpret_expression(gosubn->label);
    if (!this->priv->errors->get_code(this->priv->errors))
        this->priv->line = find_label(label);
}

// Интерпретация оператора RETURN
void interpret_return_statement(void) {
    // Локальные переменные
    GosubStackNode* gosub_node; // узел снятый со стека GOSUB

    // Возвращаемся к оператору, следующему за последним GOSUB
    if (this->priv->gosub_stack) {
        this->priv->line = this->priv->gosub_stack->program_line;
        gosub_node = this->priv->gosub_stack;
        this->priv->gosub_stack = this->priv->gosub_stack->next;
        free(gosub_node);
        --this->priv->gosub_stack_size;
    }
    // Если не было GOSUB, приведших сюда, вызываем ошибку
    else
        this->priv->errors->set_code
        (this->priv->errors, E_RETURN_WITHOUT_GOSUB, 0, this->priv->line->label);
}

// Интерпретация оператора PRINT
void interpret_print_statement(PrintStatementNode* printn) {
    // Локальные переменные
    OutputNode* outn; // текущий узел вывода
    int
        items = 0, // счетчик для обеспечения появления ошибок времени выполнения на новой строке
        result; // результат выражения

    // Выводим каждый из элементов вывода
    outn = printn->first;
    while (outn) {
        switch (outn->class) {
        case OUTPUT_STRING:
            printf("%s", outn->output.string);
            ++items;
            break;
        case OUTPUT_EXPRESSION:
            result = interpret_expression(outn->output.expression);
            if (!this->priv->errors->get_code(this->priv->errors)) {
                printf("%d", result);
                ++items;
            }
            break;
        }
        outn = outn->next;
    }

    // Выводим символ новой строки
    if (items)
        printf("\n");
    this->priv->line = this->priv->line->next;
}

// Интерпретация оператора INPUT
void interpret_input_statement(InputStatementNode* inputn) {

    // Локальные переменные
    VariableListNode* variable; // текущая переменная для ввода
    int
        value, // значение, введенное пользователем
        sign = 1, // стандартный знак
        ch = 0; // символ из потока ввода
    // Вводим каждую из переменных
    variable = inputn->first;
    while (variable) {
        do {
            if (ch == '-')
                sign = -1;
            else
                sign = 1;
            ch = getchar();
        } while (ch < '0' || ch > '9');
        value = 0;
        do {
            value = 10 * value + (ch - '0');
            if (value * sign < -32768 || value * sign > 32767)
                this->priv->errors->set_code
                (this->priv->errors, E_OVERFLOW, 0, this->priv->line->label);
            ch = getchar();
        } while (ch >= '0' && ch <= '9'
            && !this->priv->errors->get_code(this->priv->errors));
        this->priv->variables[variable->variable - 1] = sign * value;
        variable = variable->next;
    }
    // Переходим к следующему оператору после завершения
    this->priv->line = this->priv->line->next;
}


// Интерпретация отдельного оператора
void interpret_statement(StatementNode* statement) {
    // Пропускаем комментарии
    if (!statement) {
        this->priv->line = this->priv->line->next;
        return;
    }
    // Интерпретируем реальные операторы
    switch (statement->class) {
    case STATEMENT_NONE:
        break;
    case STATEMENT_LET:
        interpret_let_statement(statement->statement.letn);
        break;
    case STATEMENT_IF:
        interpret_if_statement(statement->statement.ifn);
        break;
    case STATEMENT_GOTO:
        interpret_goto_statement(statement->statement.goton);
        break;
    case STATEMENT_GOSUB:
        interpret_gosub_statement(statement->statement.gosubn);
        break;
    case STATEMENT_RETURN:
        interpret_return_statement();
        break;
    case STATEMENT_END:
        this->priv->stopped = 1;
        break;
    case STATEMENT_PRINT:
        interpret_print_statement(statement->statement.printn);
        break;
    case STATEMENT_INPUT:
        interpret_input_statement(statement->statement.inputn);
        break;
    default:
        printf("Тип оператора %d не реализован.\n", statement->class);
    }
}

// Интерпретация программы, начиная с определенной строки
static void interpret_program_from(ProgramLineNode* program_line) {
    this->priv->line = program_line;
    while (this->priv->line && !this->priv->stopped &&
        !this->priv->errors->get_code(this->priv->errors))
        interpret_statement(this->priv->line->statement);
}

 // Интерпретация программы с самого начала
static void interpret(Interpreter* interpreter, ProgramNode* program) {
    this = interpreter;
    this->priv->program = program;
    initialise_variables();
    interpret_program_from(this->priv->program->first);
}

// Уничтожение интерпретатора
static void destroy(Interpreter* interpreter) {
    if (interpreter) {
        if (interpreter->priv)
            free(interpreter->priv);
        free(interpreter);
    }
}

 // Конструктор
Interpreter* new_Interpreter(ErrorHandler* errors, LanguageOptions* options) {
    // Выделение памяти
    this = malloc(sizeof(Interpreter));
    this->priv = malloc(sizeof(InterpreterData));
    // Инициализация методов
    this->interpret = interpret;
    this->destroy = destroy;
    // Инициализация свойств
    this->priv->gosub_stack = NULL;
    this->priv->gosub_stack_size = 0;
    this->priv->stopped = 0;
    this->priv->errors = errors;
    this->priv->options = options;
    // Возвращение нового объекта
    return this;
}