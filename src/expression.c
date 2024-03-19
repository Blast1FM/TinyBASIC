// Модуль обработки выражений

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "expression.h"
#include "errors.h"

// Функции для работы с выражениями

// Конструктор для правого члена выражения
RightHandTerm* rhterm_create(void) {
    // локальные переменные
    RightHandTerm* rhterm; // новое правое выражение
    // выделение памяти и инициализация членов
    rhterm = malloc(sizeof(RightHandTerm));
    rhterm->op = EXPRESSION_OPERATOR_NONE;
    rhterm->term = NULL;
    rhterm->next = NULL;
    // возврат нового правого выражения
    return rhterm;
}

// Рекурсивный деструктор для правого члена выражения
void rhterm_destroy(RightHandTerm* rhterm) {
    if (rhterm->next)
        rhterm_destroy(rhterm->next);
    if (rhterm->term)
        term_destroy(rhterm->term);
    free(rhterm);
}

// Конструктор для выражения
ExpressionNode* expression_create(void) {
    // локальные переменные
    ExpressionNode* expression; // новое выражение
    // выделение памяти и инициализация членов
    expression = malloc(sizeof(ExpressionNode));
    expression->term = NULL;
    expression->next = NULL;
    // возврат нового выражения
    return expression;
}

// Деструктор для выражения
void expression_destroy(ExpressionNode* expression) {
    // удаление составных частей выражения
    if (expression->term)
        term_destroy(expression->term);
    if (expression->next)
        rhterm_destroy(expression->next);
    // удаление самого выражения
    free(expression);
}

// Функции для работы с множителями


// Конструктор для множителя
FactorNode* factor_create(void) {
    // локальные переменные
    FactorNode* factor; // новый множитель
    // выделение памяти и инициализация членов
    factor = malloc(sizeof(FactorNode));
    factor->class = FACTOR_NONE;
    factor->sign = SIGN_POSITIVE;
    // возврат множителя
    return factor;
}

// Деструктор для множителя
void factor_destroy(FactorNode* factor) {
    if (factor->class == FACTOR_EXPRESSION && factor->data.expression) {
        expression_destroy(factor->data.expression);
    }
    free(factor);
}

// Конструктор для правого множителя члена
RightHandFactor* rhfactor_create(void) {

    // локальные переменные
    RightHandFactor* rhfactor; // новый правый множитель члена
    // выделение памяти и инициализация членов
    rhfactor = malloc(sizeof(RightHandFactor));
    rhfactor->op = TERM_OPERATOR_NONE;
    rhfactor->factor = NULL;
    rhfactor->next = NULL;
    // возврат нового правого члена
    return rhfactor;
}

// Рекурсивный деструктор для правого множителя члена
void rhfactor_destroy(RightHandFactor* rhfactor) {
    if (rhfactor->next)
        rhfactor_destroy(rhfactor->next);
    if (rhfactor->factor)
        factor_destroy(rhfactor->factor);
    free(rhfactor);
}

// Конструктор для члена
TermNode* term_create(void) {
    // локальные переменные
    TermNode* term; // новый член
    // выделение памяти и инициализация членов
    term = malloc(sizeof(TermNode));
    term->factor = NULL;
    term->next = NULL;
    // возврат нового члена
    return term;
}

// Деструктор для члена
void term_destroy(TermNode* term) {
    // удаление составных частей члена
    if (term->factor)
        factor_destroy(term->factor);
    if (term->next)
        rhfactor_destroy(term->next);
    // удаление самого члена
    free(term);
}

