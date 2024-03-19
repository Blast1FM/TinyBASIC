// ������ ������ �� C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "statement.h"
#include "expression.h"
#include "errors.h"
#include "parser.h"
#include "options.h"
#include "generatec.h"

// ������ �����
typedef struct label {
    int number; // ����� �����
    struct label* next; // ��������� �����
} CLabel;

typedef struct {
    unsigned int input_used : 1; // true, ���� ��� ����� ��������� �����
    unsigned long int vars_used : 26; // true ��� ������ ������������ ����������
    CLabel* first_label; // ������ ������ �����
    char* code; // �������� ���� ���������������� ����
    ErrorHandler* errors; // ���������� ������ ��� ����������
    LanguageOptions* options; // ����� ����� ��� ����������
} Private;

static CProgram* this; // ������, ��� ������� ��������
static Private* data; // ��������� ������ �������
static ErrorHandler* errors; // ���������� ������
static LanguageOptions* options; // ����� �����

// ������ ������

// factor_output() ����� ������ ������ �� output_expression()
static char* output_expression(ExpressionNode* expression);

// output_statement() ����� ������ ������ �� output_if()
static char* output_statement(StatementNode* statement);

// ������� ������ 6

// ����� �������
static char* output_factor(FactorNode* factor) {

    // ��������� ����������
    char* factor_text = NULL, // ����� ����� �������
        * factor_buffer = NULL, // ��������� ����� ��� ���������� � factor_text
        * expression_text = NULL; // ����� ������������

    // ��������� �������� ����� �������
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
    // ��������� ������������� ����, ���� ����������
    if (factor_text && factor->sign == SIGN_NEGATIVE) {
        factor_buffer = malloc(strlen(factor_text) + 2);
        sprintf(factor_buffer, "-%s", factor_text);
        free(factor_text);
        factor_text = factor_buffer;
    }
    // ���������� ������������� ������������� �������
    return factor_text;
}

// ������� ������ 5

// ����� ���������� ��������� � �������
static char* output_term(TermNode* term) {
    // ��������� ����������
    char
        * term_text = NULL, // ����� ����� �����
        * factor_text = NULL, // ����� ������� �������
        operator_char; // ��������, ������������ ������ ������
    RightHandFactor* rhfactor; // ������ ������� ���������
    // �������� � ���������� �������
    if ((term_text = output_factor(term->factor))) {
        rhfactor = term->next;
        while (!errors->get_code(errors) && rhfactor) {
            // ���������� ����� ���������
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
            // �������� ������, ��������� �� ����������
            if (!errors->get_code(errors)
                && (factor_text = output_factor(rhfactor->factor))) {
                term_text = realloc(term_text,
                    strlen(term_text) + strlen(factor_text) + 2);
                sprintf(term_text, "%s%c%s", term_text, operator_char, factor_text);
                free(factor_text);
            }

            // ���� ��� ���� ���� ������ �� ���������
            rhfactor = rhfactor->next;
        }
    }
    // ���������� ����� ���������
    return term_text;
}

// ������� ������ 4
// ����� ��������� ��� ������ ���������
static char* output_expression(ExpressionNode* expression) {
    // ��������� ����������
    char
        * expression_text = NULL, // ����� ����� ���������
        * term_text = NULL, // ����� ������� �����
        operator_char; // ��������, ������������ ������ ����
    RightHandTerm* rhterm; // ������ ����� ���������
    // �������� � ���������� �����
    if ((expression_text = output_term(expression->term))) {
        rhterm = expression->next;
        while (!errors->get_code(errors) && rhterm) {

            // ���������� ����� ���������
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
            // �������� �����, ��������� �� �����������
            if (!errors->get_code(errors)
                && (term_text = output_term(rhterm->term))) {
                expression_text = realloc(expression_text,
                    strlen(expression_text) + strlen(term_text) + 2);
                sprintf(expression_text, "%s%c%s", expression_text, operator_char,
                    term_text);
                free(term_text);
            }
            // ���� ��� ���� ���� ������ �� ���������
            rhterm = rhterm->next;
        }
    }
    // ���������� ����� ���������
    return expression_text;
}

// ������� ������ 3
// ����� ��������� LET
static char* output_let(LetStatementNode* letn) {
    // ��������� ����������
    char
        * let_text = NULL, // ����� ��������� LET ��� ������
        * expression_text = NULL; // ����� ���������
    // ������ ���������
    expression_text = output_expression(letn->expression);

    // ������ �������������� ������ LET, ���� � ��� ���� ���������
    if (expression_text) {
        let_text = malloc(4 + strlen(expression_text));
        sprintf(let_text, "%c=%s;", 'a' - 1 + letn->variable, expression_text);
        free(expression_text);
        data->vars_used |= 1 << (letn->variable - 1);
    }
    // ���������� ���
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
    case RELOP_EQUAL: strcpy(op_text, "=="); break;
    case RELOP_UNEQUAL: strcpy(op_text, "!="); break;
    case RELOP_LESSTHAN: strcpy(op_text, "<"); break;
    case RELOP_LESSOREQUAL: strcpy(op_text, "<="); break;
    case RELOP_GREATERTHAN: strcpy(op_text, ">"); break;
    case RELOP_GREATEROREQUAL: strcpy(op_text, ">="); break;
    }
    // ������ �������������� ������ IF, ���� � ��� ���� ��� �����������
    if (left_text && op_text && right_text && statement_text) {
        if_text = malloc(4 + strlen(left_text) + strlen(op_text) +
            strlen(right_text) + 3 + strlen(statement_text) + 2);
        sprintf(if_text, "if (%s%s%s) {%s}", left_text, op_text, right_text,
            statement_text);
    }
    // ����������� ��������� �������
    if (left_text) free(left_text);
    if (op_text) free(op_text);
    if (right_text) free(right_text);
    if (statement_text) free(statement_text);
    // ���������� ���
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
    // ������ �������������� ������ LET, ���� � ��� ���� ���������
    if (expression_text) {
        goto_text = malloc(27 + strlen(expression_text));
        sprintf(goto_text, "label=%s; goto goto_block;", expression_text);
        free(expression_text);
    }
    // ���������� ���
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
    // ������ �������������� ������ LET, ���� � ��� ���� ���������
    if (expression_text) {
        gosub_text = malloc(12 + strlen(expression_text));
        sprintf(gosub_text, "bas_exec(%s);", expression_text);
        free(expression_text);
    }
    // ���������� ���
    return gosub_text;
}

// ����� ��������� END
static char* output_end(void) {
    char* end_text; // ������ ����� ������� END
    end_text = malloc(9);
    strcpy(end_text, "exit(0);");
    return end_text;
}

// ����� ��������� RETURN
static char* output_return(void) {
    char* return_text; // ������ ����� ������� RETURN
    return_text = malloc(8);
    strcpy(return_text, "return;");
    return return_text;
}

// ����� ��������� PRINT
static char* output_print(PrintStatementNode* printn) {
    // ��������� ����������
    char
        * format_text = NULL, // ������ ������� ��� printf
        * output_list = NULL, // ������ ������ ��� printf
        * output_text = NULL, // ��������� ������� ������
        * print_text = NULL; // ����� ��������� PRINT ��� ������
    OutputNode* output; // ������� ������� ������
    // ������������� ����� ������� � ������
    format_text = malloc(1);
    *format_text = '\0';
    output_list = malloc(1);
    *output_list = '\0';
    // ������ ������ ������� � ������
    if ((output = printn->first)) {
        do {

            // �������������� �������� ������
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
            // ���� ��������� ������� ������
        } while ((output = output->next));
    }
    // ������ ����� ������ ��������� PRINT � ��� �����������
    print_text = malloc(8 + strlen(format_text) + 3 + strlen(output_list) + 3);
    sprintf(print_text, "printf(\"%s\\n\"%s);", format_text, output_list);
    free(format_text);
    free(output_list);
    return print_text;
}

// ����� ��������� INPUT
static char* output_input(InputStatementNode* inputn) {
    // ��������� ����������
    char
        * var_text, // ����� ����� ��� ����� ����������
        * input_text; // ����� ��������� INPUT ��� ������
    VariableListNode* variable; // ������� ������� ������

    // ��������� ������ ����� ��� ������ ������������� ����������
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
    // ����������� ���������� ������
    return input_text;
}


//������� ������ 2


 // ����� ���������
static char* output_statement(StatementNode* statement) {
    // ��������� ����������
    char* output = NULL; // ��������� �����
    // ���������� ������ ����� ��� ������������
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
        strcpy(output, "�������������� ��������.");
    }
    // ���������� ������ � ����������
    return
}


// ������� ������ 1

 // ��������� ������ ���������
static void generate_line(ProgramLineNode* program_line) {
    // ��������� ����������
    CLabel
        * prior_label, // ����� ����� ������������� ������ �������
        * next_label, // ����� ����� ������������� ����� �������
        * new_label; // ����� ��� �������
    char
        label_text[12], // ����� ����� ������
        * statement_text; // ����� ���������

    // ��������� ����� ������
    if (program_line->label) {
        // ������� ����� � ������ �����
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
        // ���������� ����� � ���� ����
        sprintf(label_text, "lbl_%d:\n", program_line->label);
        data->code = realloc(data->code,
            strlen(data->code) + strlen(label_text) + 1);
        strcat(data->code, label_text);
    }
    // ��������� ��������� � ���������� ���, ���� ��� �� �����������
    statement_text = output_statement(program_line->statement);
    if (statement_text) {
        data->code = realloc(data->code,
            strlen(data->code) + strlen(statement_text) + 2);
        strcat(data->code, statement_text);
        strcat(data->code, "\n");
        free(statement_text);
    }
}


// ��������� ����� #include � #define
static void generate_includes(void) {
    // ��������� ����������
    char
        include_text[1024], // ������ ����� #include � #define
        define_text[80]; // ��������� ������ #define

    // ������ #include � #define
    strcpy(include_text, "#include <stdio.h>\n");
    strcat(include_text, "#include <stdlib.h>\n");
    sprintf(define_text, "#define E_RETURN_WITHOUT_GOSUB %d\n",
        E_RETURN_WITHOUT_GOSUB);
    strcat(include_text, define_text);

    // ���������� #include � #define � �����
    this->c_output = realloc(this->c_output, strlen(this->c_output)
        + strlen(include_text) + 1);
    strcat(this->c_output, include_text);
}

// ��������� ���������� ����������
static void generate_variables(void) {
    // ��������� ����������
    int vcount; // ������� ����������
    char
        var_text[12], // ����� ��������� ����������
        declaration[60]; // ����� ����������

    // ���� ����������
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

    // ���� ���� ����������, ��������� ���������� � �����
    if (*declaration) {
        strcat(declaration, ";\n");
        this->c_output = realloc(this->c_output, strlen(this->c_output)
            + strlen(declaration) + 1);
        strcat(this->c_output, declaration);
    }
}

// ��������� ������� bas_input
static void generate_bas_input(void) {

    // ��������� ����������
    char function_text[1024]; // ��� �������

    // ��������������� ������ �������
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

    // ���������� ������ ������� � �����
    this->c_output = realloc(this->c_output, strlen(this->c_output)
        + strlen(function_text) + 1);
    strcat(this->c_output, function_text);
}

// ��������� ������� bas_exec
static void generate_bas_exec(void) {

    // ��������� ����������
    char
        * op, // �������� ��������� ��� ������� �����
        goto_line[80], // ������ � ����� goto
        * goto_block, // ���� goto
        * function_text; // ������ ����� �������
    CLabel* label; // ��������� �� ����� ��� ���������� ����� goto

    // �����������, ����� �������� ������������ ��� ���������
    op = (options->get_line_numbers(options) == LINE_NUMBERS_OPTIONAL) ? "==" : "<=";

    // �������� ����� goto
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

    // ����������� �������
    function_text = malloc(28 + strlen(goto_block) + strlen(data->code) + 3);
    strcpy(function_text, "void bas_exec(int label) {\n");
    strcat(function_text, goto_block);
    strcat(function_text, data->code);
    strcat(function_text, "}\n");

    // ���������� ������ ������� � �����
    this->c_output = realloc(this->c_output, strlen(this->c_output) + strlen(function_text) + 1);
    strcat(this->c_output, function_text);
    free(goto_block);
    free(function_text);
}

// ��������� ������� �������
void generate_main(void) {

    // ��������� ����������
    char function_text[1024]; // ���� ����� �������

    // ������������ ������ �������
    strcpy(function_text, "int main(void) {\n");
    strcat(function_text, "bas_exec(0);\n");
    strcat(function_text, "exit(E_RETURN_WITHOUT_GOSUB);\n");
    strcat(function_text, "}\n");

    // ���������� ������ ������� � �����
    this->c_output = realloc(this->c_output, strlen(this->c_output) + strlen(function_text) + 1);
    strcat(this->c_output, function_text);
}

// ������� ��������� ���������
static void generate(CProgram* c_program, ProgramNode* program) {
    // ��������� ����������
    ProgramLineNode* program_line; // ������ ��� ���������
    // ������������� ����� �������
    this = c_program;
    data = (Private*)c_program->private_data;
    // ��������� ���� ��� �����
    program_line = program->first;
    while (program_line) {
        generate_line(program_line);
        program_line = program_line->next;
    }
    // ������ ����
    generate_includes();
    generate_variables();
    if (data->input_used)
        generate_bas_input();
    generate_bas_exec();
    generate_main();
}

// ����������
static void destroy(CProgram* c_program) {
    // ��������� ����������
    CLabel
        * current_label, // ��������� �� ����� ��� �����������
        * next_label; // ��������� �� ��������� ����� ��� �����������
    // ����������� ��������� ������
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
    // ����������� ���������������� ������
    if (this->c_output)
        free(this->c_output);
    // ����������� ���������� ���������
    free(c_program);
}

// �����������
CProgram* new_CProgram(ErrorHandler* compiler_errors, LanguageOptions* compiler_options) {

    // ��������� ������
    this = malloc(sizeof(CProgram));
    this->private_data = data = malloc(sizeof(Private));
    // ������������� �������
    this->generate = generate;
    this->destroy = destroy;
    // ������������� �������
    errors = data->errors = compiler_errors;
    options = data->options = compiler_options;
    data->input_used = 0;
    data->vars_used = 0;
    data->first_label = NULL;
    data->code = malloc(1);
    *data->code = '\0';
    this->c_output = malloc(1);
    *this->c_output = '\0';
    // ������� ��������� ���������
    return this;
}

