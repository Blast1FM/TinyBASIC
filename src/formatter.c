//  ������ ������ ��������

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "formatter.h"
#include "expression.h"
#include "errors.h"
#include "parser.h"

// ��������� ������ ����������
typedef struct formatter_data {
    ErrorHandler* errors; // ���������� ������
} FormatterData;

// ������� ����������
static Formatter* this; // ������, ��� ������� ��������


// ������ ������

// factor_output() ����� ������ ������ �� output_expression()
static char* output_expression(ExpressionNode* expression);

// output_statement() ����� ������ ������ �� output_if()
static char* output_statement(StatementNode* statement);

// ������� 


// ����� ��������� ��� �������� + ���������.
static char* output_expression(ExpressionNode* expression) {
    // ��������� ����������
    char
        * expression_text = NULL, // ����� ����� ���������
        * term_text = NULL, // ����� ������� �����
        operator_char; // ��������, ����������� ������ ����
    RightHandTerm* rhterm; // ������ ����� ���������
    // ������ � ���������� �����
    if ((expression_text = output_term(expression->term))) {
        rhterm = expression->next;
        while (!this->priv->errors->get_code(this->priv->errors) && rhterm) {

            // ����������� ������ ���������
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
            // ��������� ������, ��������� �� �����������
            if (!this->priv->errors->get_code(this->priv->errors)
                && (term_text = output_term(rhterm->term))) {
                expression_text = realloc(expression_text,
                    strlen(expression_text) + strlen(term_text) + 2);
                sprintf(expression_text, "%s%c%s", expression_text, operator_char,
                    term_text);
                free(term_text);
            }
            // ����� ��� ������ ����� ������ �� ���������
            rhterm = rhterm->next;
        }
    }
    // ������� ������ ���������
    return expression_text;
} 
// ����� ����� ��� �������� ��������� � �������
static char* output_term(TermNode* term) {
    // ��������� ����������
    char
        * term_text = NULL, // ����� ����� �����
        * factor_text = NULL, // ����� ������� ���������
        operator_char; // ��������, ����������� ������ ����� �����
    RightHandFactor* rhfactor; // ������ ��������� ���������
    // ������ � ���������� ���������
    if ((term_text = output_factor(term->factor))) {
        rhfactor = term->next;
        while (!this->priv->errors->get_code(this->priv->errors) && rhfactor) {

            // ����������� ������ ���������
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
            // ��������� ���������, ���������� �� ����������
            if (!this->priv->errors->get_code(this->priv->errors)
                && (factor_text = output_factor(rhfactor->factor))) {
                term_text = realloc(term_text,
                    strlen(term_text) + strlen(factor_text) + 2);
                sprintf(term_text, "%s%c%s", term_text, operator_char, factor_text);
                free(factor_text);
            }
            // ����� ��� ������ ����� ������ �� ���������
            rhfactor = rhfactor->next;
        }
    }
    // ������� ������ ���������
    return term_text;
}


// ����� ���������
static char* output_factor(FactorNode* factor) {

    // ��������� ����������
    char* factor_text = NULL, // ����� ����� ���������
        * factor_buffer = NULL, // ��������� ����� ��� ���������� � factor_text
        * expression_text = NULL; // ����� ������������
    // ���������� ��������� ������ ���������
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
    // ���������� �������������� �����, ���� ����������
    if (factor_text && factor->sign == SIGN_NEGATIVE) {
        factor_buffer = malloc(strlen(factor_text) + 2);
        sprintf(factor_buffer, "-%s", factor_text);
        free(factor_text);
        factor_text = factor_buffer;
    }
    // ������� �������������� ������������� ���������
    return factor_text;
}



// ����� ��������� LET
static char* output_let(LetStatementNode* letn) {
    // ��������� ����������
    char
        * let_text = NULL, // ����� ��������� LET ��� ������
        * expression_text = NULL; // ����� ���������

    // ������ ���������
    expression_text = output_expression(letn->expression);
    // ������ �������������� ������ ��������� LET, ���� ���� ���������
    if (expression_text) {
        let_text = malloc(7 + strlen(expression_text));
        sprintf(let_text, "LET %c=%s", 'A' - 1 + letn->variable, expression_text);
        free(expression_text);
    }
    // �������
    return let_text;
}

// ����� ��������� IF
static char* output_if(IfStatementNode* ifn) {
    // ��������� ����������
    char
        * if_text = NULL, // ����� ��������� IF ��� ������
        * left_text = NULL, // ����� ������ ���������
        * op_text = NULL, // ����� ���������
        * right_text = NULL, // ����� ������� ���������
        * statement_text = NULL; // ����� ��������� ���������

    // ������ ��������� � ��������� ���������
    left_text = output_expression(ifn->left);
    right_text = output_expression(ifn->right);
    statement_text = output_statement(ifn->statement);
    // ����������� ������ ���������
    op_text = malloc(3);
    switch (ifn->op) {
    case RELOP_EQUAL: strcpy(op_text, "="); break;
    case RELOP_UNEQUAL: strcpy(op_text, "<>"); break;
    case RELOP_LESSTHAN: strcpy(op_text, "<"); break;
    case RELOP_LESSOREQUAL: strcpy(op_text, "<="); break;
    case RELOP_GREATERTHAN: strcpy(op_text, ">"); break;
    case RELOP_GREATEROREQUAL: strcpy(op_text, ">="); break;
    }
    // ������ �������������� ������ ��������� IF, ���� ���� ��� �����������
    if (left_text && op_text && right_text && statement_text) {
        if_text = malloc(3 + strlen(left_text) + strlen(op_text) +
            strlen(right_text) + 6 + strlen(statement_text) + 1);
        sprintf(if_text, "IF %s%s%s THEN %s", left_text, op_text, right_text,
            statement_text);
    }
    // ������������ ��������� ��������
    if (left_text) free(left_text);
    if (op_text) free(op_text);
    if (right_text) free(right_text);
    if (statement_text) free(statement_text);
    // �������
    return if_text;
}

// ����� ��������� GOTO
static char* output_goto(GotoStatementNode* goton) {
    // ��������� ����������
    char
        * goto_text = NULL, // ����� ��������� GOTO ��� ������
        * expression_text = NULL; // ����� ���������
    // ������ ���������
    expression_text = output_expression(goton->label);
    // ������ �������������� ������ ��������� GOTO, ���� ���� ���������
    if (expression_text) {
        goto_text = malloc(6 + strlen(expression_text));
        sprintf(goto_text, "GOTO %s", expression_text);
        free(expression_text);
    }
    // �������
    return goto_text;
}


// ����� ��������� GOSUB
static char* output_gosub(GosubStatementNode* gosubn) {
    // ��������� ����������
    char
        * gosub_text = NULL, // ����� ��������� GOSUB ��� ������
        * expression_text = NULL; // ����� ���������
    // ������ ���������
    expression_text = output_expression(gosubn->label);
    // ������ �������������� ������ ��������� GOSUB, ���� ���� ���������
    if (expression_text) {
        gosub_text = malloc(7 + strlen(expression_text));
        sprintf(gosub_text, "GOSUB %s", expression_text);
        free(expression_text);
    }
    // �������
    return gosub_text;
}


// ����� ��������� END
static char* output_end(void) {
    char* end_text; // ������ ����� ������� END
    end_text = malloc(4);
    strcpy(end_text, "END");
    return end_text;
}


// ����� ��������� RETURN
static char* output_return(void) {
    char* return_text; // ������ ����� ������� RETURN
    return_text = malloc(7);
    strcpy(return_text, "RETURN");
    return return_text;
}


// ����� ��������� PRINT
static char* output_print(PrintStatementNode* printn) {
    // ��������� ����������
    char
        * print_text, // ����� ��������� PRINT ��� ������
        * output_text = NULL; // ����� �������� �������� ������
    OutputNode* output; // ������� ������� ������
    // ������������� ��������� PRINT
    print_text = malloc(6);
    strcpy(print_text, "PRINT");
    // ���������� ��������� ������
    if ((output = printn->first)) {
        do {

            // ���������� �����������
            print_text = realloc(print_text, strlen(print_text) + 2);
            strcat(print_text, output == printn->first ? " " : ",");

            // �������������� �������� ������
            switch (output->class) {
            case OUTPUT_STRING:
                output_text = malloc(strlen(output->output.string) + 3);
                sprintf(output_text, "%c%s%c", '"', output->output.string, '"');
                break;
            case OUTPUT_EXPRESSION:
                output_text = output_expression(output->output.expression);
                break;
            }

            // ���������� �������� ������
            print_text = realloc(print_text,
                strlen(print_text) + strlen(output_text) + 1);
            strcat(print_text, output_text);
            free(output_text);

            // ����� ���������� �������� ������
        } while ((output = output->next));
    }
    // ������� ���������� ������
    return print_text;
}

// ����� ��������� INPUT
static char* output_input(InputStatementNode* inputn) {
    // ��������� ����������
    char
        * input_text, // ����� ��������� INPUT ��� ������
        var_text[3]; // ��������� ������������� ������ ���������� � ������������
    VariableListNode* variable; // ������� ������� ������
    // ������������� ��������� INPUT
    input_text = malloc(6);
    strcpy(input_text, "INPUT");
    // ���������� ��������� ������
    if ((variable = inputn->first)) {
        do {
            sprintf(var_text, "%c%c",
                (variable == inputn->first) ? ' ' : ',',
                variable->variable + 'A' - 1);
            input_text = realloc(input_text, strlen(input_text) + 3);
            strcat(input_text, var_text);
        } while ((variable = variable->next));
    }
    // ������� ���������� ������
    return input_text;
}

// ����� ���������
static char* output_statement(StatementNode* statement) {
    // ��������� ����������
    char* output = NULL; // ����� ������
    // ����������� ������� ������ ��� ������������
    if (!statement)
        return NULL;
    // ������ ������ ���������
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
        strcpy(output, "����������� ��������.");
    }

    // ������� ������ ������
    return output;
}

// ����� ������ ���������
static void generate_line(ProgramLineNode* program_line) {

    // ��������� ����������
    char
        label_text[7], // ����� ����� ������
        * output = NULL, // ��������� �����
        * line_text = NULL; // ��������� ������

    // ������������� ����� ������
    if (program_line->label)
        sprintf(label_text, "%5d ", program_line->label);
    else
        strcpy(label_text, "      ");

    // ������ ������ ���������
    output = output_statement(program_line->statement);

    // ���� ��� �� �����������, ��������� ��� � ���������
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


// ��������� ������
static void generate(Formatter* formatter, ProgramNode* program) {
    // ��������� ����������
    ProgramLineNode* program_line; // ������ ��� ���������
    // ������������� ����� �������
    this = formatter;
    // ��������� ���� ��� �����
    program_line = program->first;
    while (program_line) {
        generate_line(program_line);
        program_line = program_line->next;
    }
}

// ����������� ���������� ��� �������������
static void destroy(Formatter* formatter) {
    if (formatter) {
        if (formatter->output)
            free(formatter->output);
        if (formatter->priv)
            free(formatter->priv);
        free(formatter);
    }
}

// ����������� Formatter
Formatter* new_Formatter(ErrorHandler* errors) {

    // ��������� ������
    this = malloc(sizeof(Formatter));
    this->priv = malloc(sizeof(FormatterData));

    // ������������� �������
    this->generate = generate;
    this->destroy = destroy;

    // ������������� �������
    this->output = malloc(sizeof(char));
    *this->output = '\0';
    this->priv->errors = errors;

    // ������� ������ �������
    return this;
}