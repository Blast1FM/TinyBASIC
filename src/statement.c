//Statement Handling Module

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "statement.h"


// Конструктор оператора LET
LetStatementNode* statement_create_let(void) {
    LetStatementNode* letn; // созданный узел 

    // выделение памяти и присвоение значений по умолчанию 
    letn = malloc(sizeof(LetStatementNode));
    letn->variable = 0;
    letn->expression = NULL;

    // возврат оператора LET 
    return letn;
}

// Деструктор оператора LET
void statement_destroy_let(LetStatementNode* letn) {
    if (letn->expression)
        expression_destroy(letn->expression);
    free(letn);
}

 // Конструктор оператора IF 
IfStatementNode* statement_create_if(void) {
    IfStatementNode* ifn; // созданный узел 

    // выделение памяти и присвоение значений по умолчанию 
    ifn = malloc(sizeof(IfStatementNode));
    ifn->left = ifn->right = NULL;
    ifn->op = RELOP_EQUAL;
    ifn->statement = NULL;

    // возврат оператора IF 
    return ifn;
}

//Деструктор оператора IF
void statement_destroy_if(IfStatementNode* ifn) {
    if (ifn->left)
        expression_destroy(ifn->left);
    if (ifn->right)
        expression_destroy(ifn->right);
    if (ifn->statement)
        statement_destroy(ifn->statement);
    free(ifn);
}

 // Конструктор оператора GOTO
GotoStatementNode* statement_create_goto(void) {
    GotoStatementNode* goton; // созданный оператор 

    // создание и инициализация данных 
    goton = malloc(sizeof(GotoStatementNode));
    goton->label = NULL;

    // возврат оператора GOTO 
    return goton;
}

// Деструктор оператора GOTO
void statement_destroy_goto(GotoStatementNode* goton) {
    if (goton) {
        if (goton->label)
            expression_destroy(goton->label);
        free(goton);
    }
}

// Конструктор оператора GOSUB
GosubStatementNode* statement_create_gosub(void) {
    GosubStatementNode* gosubn; // созданный оператор

    // создание и инициализация данных
    gosubn = malloc(sizeof(GosubStatementNode));
    gosubn->label = NULL;

    // возврат оператора GOSUB
    return gosubn;
}

// Деструктор оператора GOSUB
void statement_destroy_gosub(GosubStatementNode* gosubn) {
    if (gosubn) {
        if (gosubn->label)
            expression_destroy(gosubn->label);
        free(gosubn);
    }
}

// Конструктор оператора PRINT
PrintStatementNode* statement_create_print(void) {

    PrintStatementNode* printn; // созданный узел

    // выделение памяти и присвоение значений по умолчанию
    printn = malloc(sizeof(PrintStatementNode));
    printn->first = NULL;

    // возврат оператора PRINT
    return printn;
}


// Деструктор оператора PRINT
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

// Конструктор оператора INPUT
InputStatementNode* statement_create_input(void) {
    InputStatementNode* inputn; // новые данные оператора INPUT

    // выделение памяти и безопасная инициализация
    inputn = malloc(sizeof(InputStatementNode));
    inputn->first = NULL;

    // возврат созданного узла
    return inputn;
}

// Деструктор оператора INPUT
void statement_destroy_input(InputStatementNode* inputn) {

    // локальные переменные
    VariableListNode
        * variable, // текущая удаляемая переменная
        * next; // следующая удаляемая переменная

    // удаление переменных из списка переменных, затем удаление узла INPUT
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

// Конструктор оператора
// возвращает новый пустой оператор
StatementNode* statement_create(void) {

    // локальные переменные
    StatementNode* statement; // созданный оператор

    // выделение памяти и задание значений по умолчанию
    statement = malloc(sizeof(StatementNode));
    statement->class = STATEMENT_NONE;

    // возврат созданного оператора
    return statement;
}

//Деструктор оператора
void statement_destroy(StatementNode* statement) { // удаляемый оператор
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

// Конструктор строки программы
ProgramLineNode* program_line_create(void) {

    // локальные переменные
    ProgramLineNode* program_line; // созданная строка программы

    // создание и инициализация строки программы
    program_line = malloc(sizeof(ProgramLineNode));
    program_line->label = 0;
    program_line->statement = NULL;
    program_line->next = NULL;

    // возврат новой строки программы
    return program_line;
}

// Деструктор строки программы
ProgramLineNode* program_line_destroy(ProgramLineNode* program_line) {

    // локальные переменные
    ProgramLineNode* next = NULL; // следующая строка программы

    // запись следующей строки и удаление текущей
    if (program_line) {
        next = program_line->next;
        if (program_line->statement)
            statement_destroy(program_line->statement);
        free(program_line);
    }

    // возврат следующей строки
    return next;
}

// Конструктор программы
ProgramNode* program_create(void) {

    // локальные переменные
    ProgramNode* program; // новая программа

    // создание и инициализация программы
    program = malloc(sizeof(program));
    program->first = NULL;

    // возврат новой программы
    return program;
}
// Деструктор программы
void program_destroy (ProgramNode *program) {

  ProgramLineNode *program_line;

  // уничтожаем строки программы, затем саму программу
  program_line = program->first;
  while (program_line)
    program_line = program_line_destroy (program_line);
  free (program);
}
