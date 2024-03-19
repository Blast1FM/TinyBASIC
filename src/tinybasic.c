//Interpreter and Compiler Main Program


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"
#include "errors.h"
#include "parser.h"
#include "statement.h"
#include "interpret.h"
#include "formatter.h"
#include "generatec.h"

//����������� ����������
static char* input_filename = NULL; //��� �������� ����� 
static enum { // �������� � ����������� ����������� ����� 
    OUTPUT_INTERPRET, // ���������������� ��������� 
    OUTPUT_LST, // ������� ����������������� ������ 
    OUTPUT_C, // ������� ��������� �� ����� C 
    OUTPUT_EXE // ������� ����������� ���� 
} output = OUTPUT_INTERPRET;
static ErrorHandler* errors; // ������������� ���������� ������ 
static LanguageOptions* loptions; 

static void set_line_numbers(char* option) {
    if (!strncmp("optional", option, strlen(option)))
        loptions->set_line_numbers(loptions, LINE_NUMBERS_OPTIONAL);
    else if (!strncmp("implied", option, strlen(option)))
        loptions->set_line_numbers(loptions, LINE_NUMBERS_IMPLIED);
    else if (!strncmp("mandatory", option, strlen(option)))
        loptions->set_line_numbers(loptions, LINE_NUMBERS_MANDATORY);
    else
        errors->set_code(errors, E_BAD_COMMAND_LINE, 0, 0);
}

static void set_line_limit(char* option) {
    int limit; // �����, ������������ � ����� 
    if (sscanf(option, "%d", &limit))
        loptions->set_line_limit(loptions, limit);
    else
        errors->set_code(errors, E_BAD_COMMAND_LINE, 0, 0);
}

static void set_comments(char* option) {
    if (!strncmp("enabled", option, strlen(option)))
        loptions->set_comments(loptions, COMMENTS_ENABLED);
    else if (!strncmp("disabled", option, strlen(option)))
        loptions->set_comments(loptions, COMMENTS_DISABLED);
    else
        errors->set_code(errors, E_BAD_COMMAND_LINE, 0, 0);
}

static void set_output(char* option) {
    if (!strcmp("lst", option))
        output = OUTPUT_LST;
    else if (!strcmp("c", option))
        output = OUTPUT_C;
    else if (!strcmp("exe", option))
        output = OUTPUT_EXE;
    else
        errors->set_code(errors, E_BAD_COMMAND_LINE, 0, 0);
}

static void set_gosub_limit(char* option) {
    int limit; // �����, ������������ � ����� 
    if (sscanf(option, "%d", &limit))
        loptions->set_gosub_limit(loptions, limit);
    else
        errors->set_code(errors, E_BAD_COMMAND_LINE, 0, 0);
}

// ������������ ����� ��������� ������
static void set_options(int argc, char** argv) {

    int argn; // ������� ������ ����������

    // �������� �� ���� ����������
    for (argn = 1; argn < argc && !errors->get_code(errors); ++argn) {

        // ������������ ����� ������ ������
        if (!strncmp(argv[argn], "-n", 2))
            set_line_numbers(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--line-numbers=", 15))
            set_line_numbers(&argv[argn][15]);

        // ������������ ������ ������ ������
        else if (!strncmp(argv[argn], "-N", 2))
            set_line_limit(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--line-limit=", 13))
            set_line_limit(&argv[argn][13]);

        // ������������ ����� �����������
        else if (!strncmp(argv[argn], "-o", 2))
            set_comments(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--comments=", 11))
            set_comments(&argv[argn][11]);

        // ������������ ����� ������
        else if (!strncmp(argv[argn], "-O", 2))
            set_output(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--output=", 9))
            set_output(&argv[argn][9]);

        // ������������ ������ ����� GOSUB
        else if (!strncmp(argv[argn], "-g", 2))
            set_gosub_limit(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--gosub-limit=", 14))
            set_gosub_limit(&argv[argn][14]);

        // ������� ��� �����
        else if (!input_filename)
            input_filename = argv[argn];

        // ������������� ������ ��� ������������ �����
        else
            errors->set_code(errors, E_BAD_COMMAND_LINE, 0, 0);
    }
}

//������� ����������������� ���������
static void output_lst(ProgramNode* program) {

    FILE* output; // �������� ����
    char* output_filename; // ��� ��������� �����
    Formatter* formatter; // ������ ��������������

    // ����������� ����� ��������� �����
    output_filename = malloc(strlen(input_filename) + 5);
    if (output_filename) {

        // ������� �������� ����
        sprintf(output_filename, "%s.lst", input_filename);
        if ((output = fopen(output_filename, "w"))) {

            // �������� � �������� ����
            formatter = new_Formatter(errors);
            if (formatter) {
                formatter->generate(formatter, program);
                if (formatter->output)
                    fprintf(output, "%s", formatter->output);
                formatter->destroy(formatter);
            }
            fclose(output);
        }

        // ���������� ������
        else
            errors->set_code(errors, E_FILE_NOT_FOUND, 0, 0);

        // ���������� ������ ��������� ����� �����
        free(output_filename);
    }

    // ���������� ������ �������� ������
    else
        errors->set_code(errors, E_MEMORY, 0, 0);
}

// ������� �������� ���� �� ����� C
static void output_c(ProgramNode* program) {

    FILE* output; // �������� ����
    char* output_filename; // ��� ��������� �����
    CProgram* c_program; // ��������� �� ����� C

    // ������� �������� ����
    output_filename = malloc(strlen(input_filename) + 5);
    sprintf(output_filename, "%s.c", input_filename);
    if ((output = fopen(output_filename, "w"))) {

        // �������� � �������� ����
        c_program = new_CProgram(errors, loptions);
        if (c_program) {
            c_program->generate(c_program, program);
            if (c_program->c_output)
                fprintf(output, "%s", c_program->c_output);
            c_program->destroy(c_program);
        }
        fclose(output);
    }

    // ���������� ������
    else
        errors->set_code(errors, E_FILE_NOT_FOUND, 0, 0);

    // �������� ���������� ������
    free(output_filename);
}


// ������� ���������� ��� �������������� ��������� ����� �� ����� C � ����������� ����
static void output_exe(char* command, char* basic_filename) {

    char
        c_filename[256], // ��� ��������� ����� �� ����� C
        exe_filename[256], // ������� ��� ������������ �����
        final_command[1024], // ����������������� ������� �����������
        * ext, // ������� ����� ���������� � ����� �����
        * src, // ��������� �� ������ ��������� �����
        * dst; // ��������� �� ������ �������� �����

    // ����������� ���� ����� �� ����� C � ������������ �����
    sprintf(c_filename, "%s.c", basic_filename);
    strcpy(exe_filename, basic_filename);
    if ((ext = strchr(exe_filename, '.')))
        *ext = '\0';
    else
        strcat(exe_filename, ".out");

    // ���������� ������� �����������
    src = command;
    dst = final_command;
    while (*src) {
        if (!strncmp(src, "$(TARGET)", strlen("$(TARGET)"))) {
            strcpy(dst, exe_filename);
            dst += strlen(exe_filename);
            src += strlen("$(TARGET)");
        }
        else if (!strncmp(src, "$(SOURCE)", strlen("$(SOURCE)"))) {
            strcpy(dst, c_filename);
            dst += strlen(c_filename);
            src += strlen("$(SOURCE)");
        }
        else
            *(dst++) = *(src++);
    }
    *dst = '\0';

    // ���������� ������� �����������
    system(final_command);
}

int main(int argc, char** argv) {
    FILE* input; // ������� ����
    ProgramNode* program; // ����������� ���������
    ErrorCode code; // ��� ������
    Parser* parser; // ������ �������
    Interpreter* interpreter; // ������ ��������������
    char
        * error_text, // ����� ��������� �� ������
        * command; // ������� ��� ����������

    // ������������� ���������� ��������� ������
    errors = new_ErrorHandler();
    loptions = new_LanguageOptions();
    set_options(argc, argv);

    // ���� ��� ����� �� �������
    if (!input_filename) {
        printf("�������������: %s [�����] �������-����\n", argv[0]);
        errors->destroy(errors);
        loptions->destroy(loptions);
        return 0;
    }

    // ����� ���������� ������� ����
    if (!(input = fopen(input_filename, "r"))) {
        printf("������: �� ������� ������� ���� %s\n", input_filename);
        errors->destroy(errors);
        loptions->destroy(loptions);
        return E_FILE_NOT_FOUND;
    }

    // �������� ������ �������
    parser = new_Parser(errors, loptions, input);
    program = parser->parse(parser);
    parser->destroy(parser);
    fclose(input);

    // ���������� ������
    if ((code = errors->get_code(errors))) {
        error_text = errors->get_text(errors);
        printf("������ �������: %s\n", error_text);
        free(error_text);
        loptions->destroy(loptions);
        errors->destroy(errors);
        return code;
    }

    // ��������� ��������� ��������
    switch (output) {
    case OUTPUT_INTERPRET:
        interpreter = new_Interpreter(errors, loptions);
        interpreter->interpret(interpreter, program);
        interpreter->destroy(interpreter);
        if ((code = errors->get_code(errors))) {
            error_text = errors->get_text(errors);
            printf("������ ����������: %s\n", error_text);
            free(error_text);
        }
        break;
    case OUTPUT_LST:
        output_lst(program);
        break;
    case OUTPUT_C:
        output_c(program);
        break;
    case OUTPUT_EXE:
        if ((command = getenv("TBEXE"))) {
            output_c(program);
            output_exe(command, input_filename);
        }
        else
            printf("TBEXE �� ����������.\n");
        break;
    }

    // �������� � ������� �������� ����������
    program_destroy(program);
    loptions->destroy(loptions);
    errors->destroy(errors);
    return 0;

}