// ������ ��������� ���������

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "expression.h"
#include "errors.h"

// ������� ��� ������ � �����������

// ����������� ��� ������� ����� ���������
RightHandTerm* rhterm_create(void) {
    // ��������� ����������
    RightHandTerm* rhterm; // ����� ������ ���������
    // ��������� ������ � ������������� ������
    rhterm = malloc(sizeof(RightHandTerm));
    rhterm->op = EXPRESSION_OPERATOR_NONE;
    rhterm->term = NULL;
    rhterm->next = NULL;
    // ������� ������ ������� ���������
    return rhterm;
}

// ����������� ���������� ��� ������� ����� ���������
void rhterm_destroy(RightHandTerm* rhterm) {
    if (rhterm->next)
        rhterm_destroy(rhterm->next);
    if (rhterm->term)
        term_destroy(rhterm->term);
    free(rhterm);
}

// ����������� ��� ���������
ExpressionNode* expression_create(void) {
    // ��������� ����������
    ExpressionNode* expression; // ����� ���������
    // ��������� ������ � ������������� ������
    expression = malloc(sizeof(ExpressionNode));
    expression->term = NULL;
    expression->next = NULL;
    // ������� ������ ���������
    return expression;
}

// ���������� ��� ���������
void expression_destroy(ExpressionNode* expression) {
    // �������� ��������� ������ ���������
    if (expression->term)
        term_destroy(expression->term);
    if (expression->next)
        rhterm_destroy(expression->next);
    // �������� ������ ���������
    free(expression);
}

// ������� ��� ������ � �����������


// ����������� ��� ���������
FactorNode* factor_create(void) {
    // ��������� ����������
    FactorNode* factor; // ����� ���������
    // ��������� ������ � ������������� ������
    factor = malloc(sizeof(FactorNode));
    factor->class = FACTOR_NONE;
    factor->sign = SIGN_POSITIVE;
    // ������� ���������
    return factor;
}

// ���������� ��� ���������
void factor_destroy(FactorNode* factor) {
    if (factor->class == FACTOR_EXPRESSION && factor->data.expression) {
        expression_destroy(factor->data.expression);
    }
    free(factor);
}

// ����������� ��� ������� ��������� �����
RightHandFactor* rhfactor_create(void) {

    // ��������� ����������
    RightHandFactor* rhfactor; // ����� ������ ��������� �����
    // ��������� ������ � ������������� ������
    rhfactor = malloc(sizeof(RightHandFactor));
    rhfactor->op = TERM_OPERATOR_NONE;
    rhfactor->factor = NULL;
    rhfactor->next = NULL;
    // ������� ������ ������� �����
    return rhfactor;
}

// ����������� ���������� ��� ������� ��������� �����
void rhfactor_destroy(RightHandFactor* rhfactor) {
    if (rhfactor->next)
        rhfactor_destroy(rhfactor->next);
    if (rhfactor->factor)
        factor_destroy(rhfactor->factor);
    free(rhfactor);
}

// ����������� ��� �����
TermNode* term_create(void) {
    // ��������� ����������
    TermNode* term; // ����� ����
    // ��������� ������ � ������������� ������
    term = malloc(sizeof(TermNode));
    term->factor = NULL;
    term->next = NULL;
    // ������� ������ �����
    return term;
}

// ���������� ��� �����
void term_destroy(TermNode* term) {
    // �������� ��������� ������ �����
    if (term->factor)
        factor_destroy(term->factor);
    if (term->next)
        rhfactor_destroy(term->next);
    // �������� ������ �����
    free(term);
}

