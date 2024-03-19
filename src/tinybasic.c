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

//Статические переменные
static char* input_filename = NULL; //имя входного файла 
static enum { // действие с разобранным программным кодом 
    OUTPUT_INTERPRET, // интерпретировать программу 
    OUTPUT_LST, // вывести отформатированный список 
    OUTPUT_C, // вывести программу на языке C 
    OUTPUT_EXE // вывести исполняемый файл 
} output = OUTPUT_INTERPRET;
static ErrorHandler* errors; // универсальный обработчик ошибок 
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
    int limit; // число, содержащееся в опции 
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
    int limit; // число, содержащееся в опции 
    if (sscanf(option, "%d", &limit))
        loptions->set_gosub_limit(loptions, limit);
    else
        errors->set_code(errors, E_BAD_COMMAND_LINE, 0, 0);
}

// Обрабатываем опции командной строки
static void set_options(int argc, char** argv) {

    int argn; // счетчик номера аргументов

    // проходим по всем параметрам
    for (argn = 1; argn < argc && !errors->get_code(errors); ++argn) {

        // сканирование опций номера строки
        if (!strncmp(argv[argn], "-n", 2))
            set_line_numbers(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--line-numbers=", 15))
            set_line_numbers(&argv[argn][15]);

        // сканирование лимита номера строки
        else if (!strncmp(argv[argn], "-N", 2))
            set_line_limit(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--line-limit=", 13))
            set_line_limit(&argv[argn][13]);

        // сканирование опции комментария
        else if (!strncmp(argv[argn], "-o", 2))
            set_comments(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--comments=", 11))
            set_comments(&argv[argn][11]);

        // сканирование опции вывода
        else if (!strncmp(argv[argn], "-O", 2))
            set_output(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--output=", 9))
            set_output(&argv[argn][9]);

        // сканирование лимита стека GOSUB
        else if (!strncmp(argv[argn], "-g", 2))
            set_gosub_limit(&argv[argn][2]);
        else if (!strncmp(argv[argn], "--gosub-limit=", 14))
            set_gosub_limit(&argv[argn][14]);

        // принять имя файла
        else if (!input_filename)
            input_filename = argv[argn];

        // сгенерировать ошибку при некорректной опции
        else
            errors->set_code(errors, E_BAD_COMMAND_LINE, 0, 0);
    }
}

//Вывести отформатированную программу
static void output_lst(ProgramNode* program) {

    FILE* output; // выходной файл
    char* output_filename; // имя выходного файла
    Formatter* formatter; // объект форматирования

    // определение имени выходного файла
    output_filename = malloc(strlen(input_filename) + 5);
    if (output_filename) {

        // открыть выходной файл
        sprintf(output_filename, "%s.lst", input_filename);
        if ((output = fopen(output_filename, "w"))) {

            // записать в выходной файл
            formatter = new_Formatter(errors);
            if (formatter) {
                formatter->generate(formatter, program);
                if (formatter->output)
                    fprintf(output, "%s", formatter->output);
                formatter->destroy(formatter);
            }
            fclose(output);
        }

        // обработать ошибки
        else
            errors->set_code(errors, E_FILE_NOT_FOUND, 0, 0);

        // освободить память выходного имени файла
        free(output_filename);
    }

    // обработать ошибку нехватки памяти
    else
        errors->set_code(errors, E_MEMORY, 0, 0);
}

// Вывести исходный файл на языке C
static void output_c(ProgramNode* program) {

    FILE* output; // выходной файл
    char* output_filename; // имя выходного файла
    CProgram* c_program; // программа на языке C

    // открыть выходной файл
    output_filename = malloc(strlen(input_filename) + 5);
    sprintf(output_filename, "%s.c", input_filename);
    if ((output = fopen(output_filename, "w"))) {

        // записать в выходной файл
        c_program = new_CProgram(errors, loptions);
        if (c_program) {
            c_program->generate(c_program, program);
            if (c_program->c_output)
                fprintf(output, "%s", c_program->c_output);
            c_program->destroy(c_program);
        }
        fclose(output);
    }

    // обработать ошибки
    else
        errors->set_code(errors, E_FILE_NOT_FOUND, 0, 0);

    // очистить выделенную память
    free(output_filename);
}


// Вызвать компилятор для преобразования исходного файла на языке C в исполняемый файл
static void output_exe(char* command, char* basic_filename) {

    char
        c_filename[256], // имя исходного файла на языке C
        exe_filename[256], // базовое имя исполняемого файла
        final_command[1024], // сконструированная команда компилятора
        * ext, // позиция точки расширения в имени файла
        * src, // указатель на строку исходного файла
        * dst; // указатель на строку целевого файла

    // определение имен файла на языке C и исполняемого файла
    sprintf(c_filename, "%s.c", basic_filename);
    strcpy(exe_filename, basic_filename);
    if ((ext = strchr(exe_filename, '.')))
        *ext = '\0';
    else
        strcat(exe_filename, ".out");

    // построение команды компилятора
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

    // выполнение команды компилятора
    system(final_command);
}

int main(int argc, char** argv) {
    FILE* input; // входной файл
    ProgramNode* program; // разобранная программа
    ErrorCode code; // код ошибки
    Parser* parser; // объект парсера
    Interpreter* interpreter; // объект интерпретатора
    char
        * error_text, // текст сообщения об ошибке
        * command; // команда для компиляции

    // интерпретация аргументов командной строки
    errors = new_ErrorHandler();
    loptions = new_LanguageOptions();
    set_options(argc, argv);

    // Если имя файла не указано
    if (!input_filename) {
        printf("Использование: %s [ОПЦИИ] ВХОДНОЙ-ФАЙЛ\n", argv[0]);
        errors->destroy(errors);
        loptions->destroy(loptions);
        return 0;
    }

    // иначе попытаться открыть файл
    if (!(input = fopen(input_filename, "r"))) {
        printf("Ошибка: не удается открыть файл %s\n", input_filename);
        errors->destroy(errors);
        loptions->destroy(loptions);
        return E_FILE_NOT_FOUND;
    }

    // получить дерево разбора
    parser = new_Parser(errors, loptions, input);
    program = parser->parse(parser);
    parser->destroy(parser);
    fclose(input);

    // обработать ошибки
    if ((code = errors->get_code(errors))) {
        error_text = errors->get_text(errors);
        printf("Ошибка разбора: %s\n", error_text);
        free(error_text);
        loptions->destroy(loptions);
        errors->destroy(errors);
        return code;
    }

    // выполнить требуемое действие
    switch (output) {
    case OUTPUT_INTERPRET:
        interpreter = new_Interpreter(errors, loptions);
        interpreter->interpret(interpreter, program);
        interpreter->destroy(interpreter);
        if ((code = errors->get_code(errors))) {
            error_text = errors->get_text(errors);
            printf("Ошибка выполнения: %s\n", error_text);
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
            printf("TBEXE не установлен.\n");
        break;
    }

    // очистить и вернуть успешное завершение
    program_destroy(program);
    loptions->destroy(loptions);
    errors->destroy(errors);
    return 0;

}