//Statement Handling Module

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "statement.h"


// ����������� ��������� LET
LetStatementNode* statement_create_let(void) {
    LetStatementNode* letn; // ��������� ���� 

    // ��������� ������ � ���������� �������� �� ��������� 
    letn = malloc(sizeof(LetStatementNode));
    letn->variable = 0;
    letn->expression = NULL;

    // ������� ��������� LET 
    return letn;
}

// ���������� ��������� LET
void statement_destroy_let(LetStatementNode* letn) {
    if (letn->expression)
        expression_destroy(letn->expression);
    free(letn);
}

 // ����������� ��������� IF 
IfStatementNode* statement_create_if(void) {
    IfStatementNode* ifn; // ��������� ���� 

    // ��������� ������ � ���������� �������� �� ��������� 
    ifn = malloc(sizeof(IfStatementNode));
    ifn->left = ifn->right = NULL;
    ifn->op = RELOP_EQUAL;
    ifn->statement = NULL;

    // ������� ��������� IF 
    return ifn;
}

//���������� ��������� IF
void statement_destroy_if(IfStatementNode* ifn) {
    if (ifn->left)
        expression_destroy(ifn->left);
    if (ifn->right)
        expression_destroy(ifn->right);
    if (ifn->statement)
        statement_destroy(ifn->statement);
    free(ifn);
}

 // ����������� ��������� GOTO
GotoStatementNode* statement_create_goto(void) {
    GotoStatementNode* goton; // ��������� �������� 

    // �������� � ������������� ������ 
    goton = malloc(sizeof(GotoStatementNode));
    goton->label = NULL;

    // ������� ��������� GOTO 
    return goton;
}

// ���������� ��������� GOTO
void statement_destroy_goto(GotoStatementNode* goton) {
    if (goton) {
        if (goton->label)
            expression_destroy(goton->label);
        free(goton);
    }
}

// ����������� ��������� GOSUB
GosubStatementNode* statement_create_gosub(void) {
    GosubStatementNode* gosubn; // ��������� ��������

    // �������� � ������������� ������
    gosubn = malloc(sizeof(GosubStatementNode));
    gosubn->label = NULL;

    // ������� ��������� GOSUB
    return gosubn;
}

// ���������� ��������� GOSUB
void statement_destroy_gosub(GosubStatementNode* gosubn) {
    if (gosubn) {
        if (gosubn->label)
            expression_destroy(gosubn->label);
        free(gosubn);
    }
}

// ����������� ��������� PRINT
PrintStatementNode* statement_create_print(void) {

    PrintStatementNode* printn; // ��������� ����

    // ��������� ������ � ���������� �������� �� ���������
    printn = malloc(sizeof(PrintStatementNode));
    printn->first = NULL;

    // ������� ��������� PRINT
    return printn;
}


// ���������� ��������� PRINT
void statement_destroy_print(PrintStatementNode* printn) {
    OutputNode* current, * next;
    current = printn->first;
    while (current) {
        next = current->next;
        if (current->class == OUTPUT_STRING)
            free(current->output.string);
        else if (current->class == OUTPUT_EXPRESSION)
            expression_destroy(current->output.expression);
        free(current);
        current = next;
    }
    free(printn);
}

// ����������� ��������� INPUT
InputStatementNode* statement_create_input(void) {
    InputStatementNode* inputn; // ����� ������ ��������� INPUT

    // ��������� ������ � ���������� �������������
    inputn = malloc(sizeof(InputStatementNode));
    inputn->first = NULL;

    // ������� ���������� ����
    return inputn;
}

// ���������� ��������� INPUT
void statement_destroy_input(InputStatementNode* inputn) {

    // ��������� ����������
    VariableListNode
        * variable, // ������� ��������� ����������
        * next; // ��������� ��������� ����������

    // �������� ���������� �� ������ ����������, ����� �������� ���� INPUT
    if (inputn) {
        variable = inputn->first;
        while (variable) {
            next = variable->next;
            free(variable);
            variable = next;
        }
        free(inputn);
    }
}

// ����������� ���������
// ���������� ����� ������ ��������
StatementNode* statement_create(void) {

    // ��������� ����������
    StatementNode* statement; // ��������� ��������

    // ��������� ������ � ������� �������� �� ���������
    statement = malloc(sizeof(StatementNode));
    statement->class = STATEMENT_NONE;

    // ������� ���������� ���������
    return statement;
}

//���������� ���������
void statement_destroy(StatementNode* statement) { // ��������� ��������
    switch (statement->class) {
    case STATEMENT_LET:
        statement_destroy_let(statement->statement.letn);
        break;
    case STATEMENT_PRINT:
        statement_destroy_print(statement->statement.printn);
        break;
    case STATEMENT_INPUT:
        statement_destroy_input(statement->statement.inputn);
        break;
    case STATEMENT_IF:
        statement_destroy_if(statement->statement.ifn);
        break;
    case STATEMENT_GOTO:
        statement_destroy_goto(statement->statement.goton);
        break;
    case STATEMENT_GOSUB:
        statement_destroy_gosub(statement->statement.gosubn);
        break;
    default:
        break;
    }
    free(statement);
}

// ����������� ������ ���������
ProgramLineNode* program_line_create(void) {

    // ��������� ����������
    ProgramLineNode* program_line; // ��������� ������ ���������

    // �������� � ������������� ������ ���������
    program_line = malloc(sizeof(ProgramLineNode));
    program_line->label = 0;
    program_line->statement = NULL;
    program_line->next = NULL;

    // ������� ����� ������ ���������
    return program_line;
}

// ���������� ������ ���������
ProgramLineNode* program_line_destroy(ProgramLineNode* program_line) {

    // ��������� ����������
    ProgramLineNode* next = NULL; // ��������� ������ ���������

    // ������ ��������� ������ � �������� �������
    if (program_line) {
        next = program_line->next;
        if (program_line->statement)
            statement_destroy(program_line->statement);
        free(program_line);
    }

    // ������� ��������� ������
    return next;
}

// ����������� ���������
ProgramNode* program_create(void) {

    // ��������� ����������
    ProgramNode* program; // ����� ���������

    // �������� � ������������� ���������
    program = malloc(sizeof(program));
    program->first = NULL;

    // ������� ����� ���������
    return program;
}
// ���������� ���������
void program_destroy (ProgramNode *program) {

  ProgramLineNode *program_line;

  // ���������� ������ ���������, ����� ���� ���������
  program_line = program->first;
  while (program_line)
    program_line = program_line_destroy (program_line);
  free (program);
}
