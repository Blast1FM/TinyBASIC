// ������ ��������������

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpret.h"
#include "errors.h"
#include "options.h"
#include "statement.h"

static int interpret_expression(ExpressionNode* expression);
static void interpret_statement(StatementNode* statement);

 // ���� GOSUB
typedef struct gosub_stack_node GosubStackNode;
typedef struct gosub_stack_node {
    ProgramLineNode* program_line; // ������ ����� GOSUB
    GosubStackNode* next; // ���� ����� ��� ����������� GOSUB
} GosubStackNode;

typedef struct interpreter_data {
    ProgramNode* program; // ��������� ��� �������������
    ProgramLineNode* line; // ������� ����������� ������
    GosubStackNode* gosub_stack; // ������� ����� GOSUB
    int gosub_stack_size; // ���������� ������� � ����� GOSUB
    int variables[26]; // �������� ����������
    int stopped; // ����������� � 1 ��� ����������� END
    ErrorHandler* errors; // ���������� ������
    LanguageOptions* options; // ����� �����
} InterpreterData;

static Interpreter* this; // ������, � ������� �� ��������

 // ������ ��������� ��� ��������������
static int interpret_factor(FactorNode* factor) {
    // ��������� ����������
    int result_store = 0; // ��������� ������ ���������
    // �������� ������ ���������
    switch (factor->class) {
        // ������� ����������
    case FACTOR_VARIABLE:
        result_store = this->priv->variables[factor->data.variable - 1] *
            (factor->sign == SIGN_POSITIVE ? 1 : -1);
        break;
        // ������������� ���������
    case FACTOR_VALUE:
        result_store = factor->data.value *
            (factor->sign == SIGN_POSITIVE ? 1 : -1);
        break;
        // ���������
    case FACTOR_EXPRESSION:
        result_store = interpret_expression(factor->data.expression) *
            (factor->sign == SIGN_POSITIVE ? 1 : -1);
        break;
        // ��� ���������� ������ � ������ ������������ ������������ �������
    default:
        this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, 0,
            this->priv->line->label);
    }
    // ��������� ��������� � ���������� ���
    if (result_store < -32768 || result_store > 32767)
        this->priv->errors->set_code(this->priv->errors, E_OVERFLOW, 0,
            this->priv->line->label);
    return result_store;
}

// ������ ���������
static int interpret_term(TermNode* term) {
    // ��������� ����������
    int result_store; // ��������� ����������
    RightHandFactor* rhfactor; // ��������� �� ����������� ���� ������ �����
    int divisor; // ������������ ��� �������� ������� �� 0 ����� ��������
    // ��������� ��������� ������� ���������
    result_store = interpret_factor(term->factor);
    rhfactor = term->next;
    // ������������ ��������� � ������������ � ����������������� �����������
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
    // ���������� ���������
    return result_store;
}

// ������ ��������� ��� ��������������
static int interpret_expression(ExpressionNode* expression) {

    // ��������� ����������
    int result_store; // ��������� ����������
    RightHandTerm* rhterm; // ��������� �� ����������� ���� ������ �����

    // ��������� ��������� ������� �����
    result_store = interpret_term(expression->term);
    rhterm = expression->next;

    // ������������ ��������� � ������������ � ����������������� �������
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
    // ���������� ���������
    return result_store;
}

// ����� ������ ��������� �� �� �����
static ProgramLineNode* find_label(int jump_label) {
    // ��������� ����������
    ProgramLineNode* ptr, * found = NULL; // ������, ������� �� �������������
    // �����
    for (ptr = this->priv->program->first; ptr && !found; ptr = ptr->next)
        if (ptr->label == jump_label)
            found = ptr;
        else if (ptr->label >= jump_label &&
            this->priv->options->get_line_numbers(this->priv->options) !=
            LINE_NUMBERS_OPTIONAL)
            found = ptr;
    // ��������� �� ������ � ���������� ���������
    if (!found)
        this->priv->errors->set_code(this->priv->errors, E_INVALID_LINE_NUMBER, 0,
            this->priv->line->label);
    return found;
}

//������� 1 ������

 // ������������� ����������
static void initialise_variables(void) {
    int count; // ������� ��� ����������
    for (count = 0; count < 26; ++count) {
        this->priv->variables[count] = 0;
    }
}

// ������������� ��������� LET
void interpret_let_statement(LetStatementNode* letn) {
    this->priv->variables[letn->variable - 1] =
        interpret_expression(letn->expression);
    this->priv->line = this->priv->line->next;
}

// ������������� ��������� IF
void interpret_if_statement(IfStatementNode* ifn) {
    // ��������� ����������
    int left, right, comparison; // ���������� ���������
    // �������� ���������
    left = interpret_expression(ifn->left);
    right = interpret_expression(ifn->right);
    // ������ ���������
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
    // ��������� �������� ��������
    if (comparison && !this->priv->errors->get_code(this->priv->errors))
        interpret_statement(ifn->statement);
    else
        this->priv->line = this->priv->line->next;
}


// ������������� ��������� GOTO
void interpret_goto_statement(GotoStatementNode* goton) {
    int label; // ����� ������, � ������� �������
    label = interpret_expression(goton->label);
    if (!this->priv->errors->get_code(this->priv->errors))
        this->priv->line = find_label(label);
}

// ������������� ��������� GOSUB
void interpret_gosub_statement(GosubStatementNode* gosubn) {
    // ��������� ����������
    GosubStackNode* gosub_node; // ��������� �� ������ ���������, � ������� ����� ���������
    int label; // ����� ������, � ������� �������
    // ������� ����� ���� �� ����� GOSUB
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

    // ��������� � ����������� ������������
    if (!this->priv->errors->get_code(this->priv->errors))
        label = interpret_expression(gosubn->label);
    if (!this->priv->errors->get_code(this->priv->errors))
        this->priv->line = find_label(label);
}

// ������������� ��������� RETURN
void interpret_return_statement(void) {
    // ��������� ����������
    GosubStackNode* gosub_node; // ���� ������ �� ����� GOSUB

    // ������������ � ���������, ���������� �� ��������� GOSUB
    if (this->priv->gosub_stack) {
        this->priv->line = this->priv->gosub_stack->program_line;
        gosub_node = this->priv->gosub_stack;
        this->priv->gosub_stack = this->priv->gosub_stack->next;
        free(gosub_node);
        --this->priv->gosub_stack_size;
    }
    // ���� �� ���� GOSUB, ��������� ����, �������� ������
    else
        this->priv->errors->set_code
        (this->priv->errors, E_RETURN_WITHOUT_GOSUB, 0, this->priv->line->label);
}

// ������������� ��������� PRINT
void interpret_print_statement(PrintStatementNode* printn) {
    // ��������� ����������
    OutputNode* outn; // ������� ���� ������
    int
        items = 0, // ������� ��� ����������� ��������� ������ ������� ���������� �� ����� ������
        result; // ��������� ���������

    // ������� ������ �� ��������� ������
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

    // ������� ������ ����� ������
    if (items)
        printf("\n");
    this->priv->line = this->priv->line->next;
}

// ������������� ��������� INPUT
void interpret_input_statement(InputStatementNode* inputn) {

    // ��������� ����������
    VariableListNode* variable; // ������� ���������� ��� �����
    int
        value, // ��������, ��������� �������������
        sign = 1, // ����������� ����
        ch = 0; // ������ �� ������ �����
    // ������ ������ �� ����������
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
    // ��������� � ���������� ��������� ����� ����������
    this->priv->line = this->priv->line->next;
}


// ������������� ���������� ���������
void interpret_statement(StatementNode* statement) {
    // ���������� �����������
    if (!statement) {
        this->priv->line = this->priv->line->next;
        return;
    }
    // �������������� �������� ���������
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
        printf("��� ��������� %d �� ����������.\n", statement->class);
    }
}

// ������������� ���������, ������� � ������������ ������
static void interpret_program_from(ProgramLineNode* program_line) {
    this->priv->line = program_line;
    while (this->priv->line && !this->priv->stopped &&
        !this->priv->errors->get_code(this->priv->errors))
        interpret_statement(this->priv->line->statement);
}

 // ������������� ��������� � ������ ������
static void interpret(Interpreter* interpreter, ProgramNode* program) {
    this = interpreter;
    this->priv->program = program;
    initialise_variables();
    interpret_program_from(this->priv->program->first);
}

// ����������� ��������������
static void destroy(Interpreter* interpreter) {
    if (interpreter) {
        if (interpreter->priv)
            free(interpreter->priv);
        free(interpreter);
    }
}

 // �����������
Interpreter* new_Interpreter(ErrorHandler* errors, LanguageOptions* options) {
    // ��������� ������
    this = malloc(sizeof(Interpreter));
    this->priv = malloc(sizeof(InterpreterData));
    // ������������� �������
    this->interpret = interpret;
    this->destroy = destroy;
    // ������������� �������
    this->priv->gosub_stack = NULL;
    this->priv->gosub_stack_size = 0;
    this->priv->stopped = 0;
    this->priv->errors = errors;
    this->priv->options = options;
    // ����������� ������ �������
    return this;
}