//Parser module

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "errors.h"
#include "options.h"
#include "token.h"
#include "tokeniser.h"
#include "parser.h"
#include "expression.h"

// ������� parse_expression() ����� ������ ������ �� ������� parse_factor()
static ExpressionNode* parse_expression(void);

// ������� parse_statement() ����� ������ ������ �� ������� parse_if_statement()
static StatementNode* parse_statement(void);

// ��������� ������ 
typedef struct parser_data {
    int last_label; // ��������� ����� ������, � ������� ����������� 
    int current_line; // ��������� ����������� �������� ������ 
    int end_of_file; // ������ � ����� ����� 
    Token* stored_token; // ������� ����������� ����� 
    TokenStream* stream; // ������� ����� 
    ErrorHandler* errors; // ���������� ������ �������� 
    LanguageOptions* options; // ��������� �����
} ParserData;

// ��������������� ����������
static Parser* this; // ������� ������

// ��������� ������

// ��������� ��������� ����� ��� ������� �� ������ ������� ����������� ������� ��� ������������.

static Token* get_token_to_parse() {
    Token* token; // ����� ��� ��������

    // �������� ����� ����� �������� ��� ������
    if (this->priv->stored_token) {
        token = this->priv->stored_token;
        this->priv->stored_token = NULL;
    }
    else
        token = this->priv->stream->next(this->priv->stream);

    // ��������� ����� ������, ��������� EOF � ������� �����
    this->priv->current_line = token->get_line(token);
    if (token->get_class(token) == TOKEN_EOF)
        this->priv->end_of_file = !0;
    return token;
}

// ������ �������
static FactorNode* parse_factor(void) {
    Token* token; // ����� ��� ������
    FactorNode* factor = NULL; // ������, ������� �� ���������
    ExpressionNode* expression = NULL; // ��������� � ������� (���� ����)
    int start_line; // ������, �� ������� ��������� ���� ������

    // ������������� ������� � ��������� ���������� ������
    factor = factor_create();
    token = get_token_to_parse();
    start_line = token->get_line(token);

    // ������������� �����
    if (token->get_class(token) == TOKEN_PLUS ||
        token->get_class(token) == TOKEN_MINUS) {
        factor->sign = (token->get_class(token) == TOKEN_PLUS) ? SIGN_POSITIVE : SIGN_NEGATIVE;
        token->destroy(token);
        token = get_token_to_parse();
    }

    // ������������� �����
    if (token->get_class(token) == TOKEN_NUMBER) {
        factor->class = FACTOR_VALUE;
        factor->data.value = atoi(token->get_content(token));
        if (factor->data.value < -32768 || factor->data.value > 32767)
            this->priv->errors->set_code(this->priv->errors, E_OVERFLOW, start_line, this->priv->last_label);
        token->destroy(token);
    }

    // ������������� ����������
    else if (token->get_class(token) == TOKEN_VARIABLE) {
        factor->class = FACTOR_VARIABLE;
        factor->data.variable = (int)(*token->get_content(token)) & 0x1F;
        token->destroy(token);
    }

    // ������������� ��������� � �������
    else if (token->get_class(token) == TOKEN_LEFT_PARENTHESIS) {

        // ������ ��������� � ������� � ���������� �������
        token->destroy(token);
        expression = parse_expression();
        if (expression) {
            token = get_token_to_parse();
            if (token->get_class(token) == TOKEN_RIGHT_PARENTHESIS) {
                factor->class = FACTOR_EXPRESSION;
                factor->data.expression = expression;
            }
            else {
                this->priv->errors->set_code(this->priv->errors, E_MISSING_RIGHT_PARENTHESIS, start_line, this->priv->last_label);
                factor_destroy(factor);
                factor = NULL;
                expression_destroy(expression);
            }
            token->destroy(token);
        }

        // ������� ����� ������������� ��������� � �������
        else {
            this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, token->get_line(token), this->priv->last_label);
            token->destroy(token);
            factor_destroy(factor);
            factor = NULL;
        }
    }

    // ��������� ������ ������
    else {
        this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, token->get_line(token), this->priv->last_label);
        factor_destroy(factor);
        token->destroy(token);
        factor = NULL;
    }

    // ������� �������
    return factor;
}

// ������ �����
static TermNode* parse_term(void) {

    TermNode* term = NULL; // ����, ������� �� ������
    FactorNode* factor = NULL; // ������������ ������
    RightHandFactor* rhptr = NULL; // ���������� ������ ������
    RightHandFactor* rhfactor = NULL; // ������������ ������ ������
    Token* token = NULL; // �����, ����������� ��� ������ ���������

    // ��������� ������ ������
    if ((factor = parse_factor())) {
        term = term_create();
        term->factor = factor;
        term->next = NULL;

        // ���� ����������� �������
        while ((token = get_token_to_parse()) &&
            !this->priv->errors->get_code(this->priv->errors) &&
            (token->get_class(token) == TOKEN_MULTIPLY ||
                token->get_class(token) == TOKEN_DIVIDE)) {

            // ��������� ���� � ������
            rhfactor = rhfactor_create();
            rhfactor->op = (token->get_class(token) == TOKEN_MULTIPLY) ? TERM_OPERATOR_MULTIPLY : TERM_OPERATOR_DIVIDE;
            if ((rhfactor->factor = parse_factor())) {
                rhfactor->next = NULL;
                if (rhptr)
                    rhptr->next = rhfactor;
                else
                    term->next = rhfactor;
                rhptr = rhfactor;
            }
            else {
                rhfactor_destroy(rhfactor);
                if (!this->priv->errors->get_code(this->priv->errors))
                    this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, token->get_line(token), this->priv->last_label);
            }

            // ������� ������
            token->destroy(token);
        }

        // �� ��������� �� ������� �����; ���������� �����
        this->priv->stored_token = token;
    }

    // ���������� ����������� ����, ���� ����
    return term;
}

// ������ ���������
static ExpressionNode* parse_expression(void) {
    ExpressionNode* expression = NULL; // ���������, ������� �� ������
    TermNode* term; // ������������ ����
    RightHandTerm* rhterm = NULL; // ������������ ������ ����
    RightHandTerm* rhptr = NULL; // ��������� �� ���������� ������ ����
    Token* token; // �����, ����������� ��� ������������ ������ ������

    // ��������� ������ ����
    if ((term = parse_term())) {
        expression = expression_create();
        expression->term = term;
        expression->next = NULL;

        // ���� ����������� �����
        while ((token = get_token_to_parse()) &&
            !this->priv->errors->get_code(this->priv->errors) &&
            (token->get_class(token) == TOKEN_PLUS ||
                token->get_class(token) == TOKEN_MINUS ||
                token->get_class(token) == TOKEN_RND)) {

            // ��������� ���� � ����
            rhterm = rhterm_create();
            rhterm->op = (token->get_class(token) == TOKEN_PLUS) ? EXPRESSION_OPERATOR_PLUS :
                (token->get_class(token) == TOKEN_MINUS) ? EXPRESSION_OPERATOR_MINUS : EXPRESSION_OPERATOR_RND;
            if ((rhterm->term = parse_term())) {
                rhterm->next = NULL;
                if (rhptr)
                    rhptr->next = rhterm;
                else
                    expression->next = rhterm;
                rhptr = rhterm;
            }
            else {
                rhterm_destroy(rhterm);
                if (!this->priv->errors->get_code(this->priv->errors))
                    this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, token->get_line(token), this->priv->last_label);
            }

            // ������� ������
            token->destroy(token);
        }

        // �� ��������� �� ������� �����; ���������� �����
        this->priv->stored_token = token;
    }

    // ���������� ����������� ���������, ���� ����
    return expression;
}

/// ������ �������� ����� ������ � ������������ � ����������� �����.
// ��� ����� ��������������, ���� ������ �� ����� ��������� �����.
static int generate_default_label(void) {
    if (this->priv->options->get_line_numbers(this->priv->options) == LINE_NUMBERS_IMPLIED)
        return this->priv->last_label + 1;
    else
        return 0;
}

// �������� ���������� �������� ����� ������ � ������������ � ����������� �����
static int validate_line_label(int label) {

    // �������� ����� ������ ������ ���� ��������������� � � �������� ��������� �����������
    if (label < 0 || label > this->priv->options->get_line_limit(this->priv->options))
        return 0;

    // �������� ����� ������ ������ ���� ���������, ���� ��� �������������
    if (label == 0 && this->priv->options->get_line_numbers(this->priv->options) != LINE_NUMBERS_OPTIONAL)
        return 0;

    // �������� ����� ������ ������ ���� ������������, ���� ��� �������������
    if (label <= this->priv->last_label && this->priv->options->get_line_numbers(this->priv->options) != LINE_NUMBERS_OPTIONAL)
        return 0;

    // ���� ��� �������� ���� ��������, �������� ����� ������ �������������
    return 1;
}

// ������ ��������� LET
static StatementNode* parse_let_statement(void) {
    Token* token; // ������, ����������� ��� ����� ��������� LET
    int line; // ������, ���������� ����� LET
    StatementNode* statement; // ����� ��������

    // ������������� ���������
    statement = statement_create();
    statement->class = STATEMENT_LET;
    statement->statement.letn = statement_create_let();
    line = this->priv->stream->get_line(this->priv->stream);

    // ����������� ����������, ������� �� �����������
    token = get_token_to_parse();
    if (token->get_class(token) != TOKEN_VARIABLE) {
        this->priv->errors->set_code(this->priv->errors, E_INVALID_VARIABLE, line, this->priv->last_label);
        statement_destroy(statement);
        token->destroy(token);
        return NULL;
    }
    statement->statement.letn->variable = *(token->get_content(token)) & 0x1f;

    // ��������� "="
    token->destroy(token);
    token = get_token_to_parse();
    if (token->get_class(token) != TOKEN_EQUAL) {
        this->priv->errors->set_code(this->priv->errors, E_INVALID_ASSIGNMENT, line, this->priv->last_label);
        statement_destroy(statement);
        token->destroy(token);
        return NULL;
    }
    token->destroy(token);

    // ��������� ���������
    statement->statement.letn->expression = parse_expression();
    if (!statement->statement.letn->expression) {
        this->priv->errors->set_code(this->priv->errors, E_INVALID_EXPRESSION, line, this->priv->last_label);
        statement_destroy(statement);
        return NULL;
    }

    // ����������� ���������
    return statement;
}


// ������ ��������� IF
static StatementNode* parse_if_statement(void) {

    Token* token; // ������, ����������� ��� ����� ���������
    StatementNode* statement; // �������� IF

    // ������������� ���������
    statement = statement_create();
    statement->class = STATEMENT_IF;
    statement->statement.ifn = statement_create_if();

    // ������ ������� ���������
    statement->statement.ifn->left = parse_expression();

    // ������ ���������
    if (!this->priv->errors->get_code(this->priv->errors)) {
        token = get_token_to_parse();
        switch (token->get_class(token)) {
        case TOKEN_EQUAL:
            statement->statement.ifn->op = RELOP_EQUAL;
            break;
        case TOKEN_UNEQUAL:
            statement->statement.ifn->op = RELOP_UNEQUAL;
            break;
        case TOKEN_LESSTHAN:
            statement->statement.ifn->op = RELOP_LESSTHAN;
            break;
        case TOKEN_LESSOREQUAL:
            statement->statement.ifn->op = RELOP_LESSOREQUAL;
            break;
        case TOKEN_GREATERTHAN:
            statement->statement.ifn->op = RELOP_GREATERTHAN;
            break;
        case TOKEN_GREATEROREQUAL:
            statement->statement.ifn->op = RELOP_GREATEROREQUAL;
            break;
        default:
            this->priv->errors->set_code(this->priv->errors, E_INVALID_OPERATOR, token->get_line(token), this->priv->last_label);
        }
        token->destroy(token);
    }

    // ������ ������� ���������
    if (!this->priv->errors->get_code(this->priv->errors))
        statement->statement.ifn->right = parse_expression();

    // ������ ��������� ����� THEN
    if (!this->priv->errors->get_code(this->priv->errors)) {
        token = get_token_to_parse();
        if (token->get_class(token) != TOKEN_THEN)
            this->priv->errors->set_code(this->priv->errors, E_THEN_EXPECTED, token->get_line(token), this->priv->last_label);
        token->destroy(token);
    }

    // ������ ��������� ���������
    if (!this->priv->errors->get_code(this->priv->errors))
        statement->statement.ifn->statement = parse_statement();

    // ����������� ��������� ���������, ���� ��������� ������
    if (this->priv->errors->get_code(this->priv->errors)) {
        statement_destroy(statement);
        statement = NULL;
    }

    // ����������� ���������
    return statement;
}

// ������ ��������� GOTO
static StatementNode* parse_goto_statement(void) {
    StatementNode* statement; // �������� GOTO

    // ������������� ���������
    statement = statement_create();
    statement->class = STATEMENT_GOTO;
    statement->statement.goton = statement_create_goto();

    // ������ ��������� � ������ ������
    if (!(statement->statement.goton->label = parse_expression())) {
        statement_destroy(statement);
        statement = NULL;
    }

    // ����������� ������ ���������
    return statement;
}

// ������ ��������� GOSUB
static StatementNode* parse_gosub_statement(void) {

    // ��������� ����������
    StatementNode* statement; // �������� GOSUB

    // ������������� ���������
    statement = statement_create();
    statement->class = STATEMENT_GOSUB;
    statement->statement.gosubn = statement_create_gosub();

    // ������ ��������� � ������ ������
    if (!(statement->statement.gosubn->label = parse_expression())) {
        statement_destroy(statement);
        statement = NULL;
    }

    // ����������� ������ ���������
    return statement;
}

// ������ ��������� RETURN
static StatementNode* parse_return_statement(void) {
    StatementNode* statement; // �������� RETURN
    statement = statement_create();
    statement->class = STATEMENT_RETURN;
    return statement;
}

// ������ ��������� END
static StatementNode* parse_end_statement(void) {
    StatementNode* statement = NULL; // �������� END
    statement = statement_create();
    statement->class = STATEMENT_END;
    return statement;
}


//
 * Parse a PRINT statement
 * returns:
 *   StatementNode*          The statement assembled
 
static StatementNode *parse_print_statement (void) {

  // local variables 
  Token *token = NULL; // tokens read as part of the statement 
  StatementNode *statement; // the statement we're building 
  int line; // line containing the PRINT token 
  OutputNode
    *nextoutput = NULL, // the next output node we're parsing 
    *lastoutput = NULL; // the last output node we parsed 
  ExpressionNode *expression; // a parsed expression 

  // initialise the statement 
  statement = statement_create ();
  statement->class = STATEMENT_PRINT;
  statement->statement.printn = statement_create_print ();
  line = this->priv->stream->get_line (this->priv->stream);

  // main loop for parsing the output list 
  do {

    // discard a previous comma, and read the next output value 
    if (token)
      token->destroy (token);
    token = get_token_to_parse ();

    // process a premature end of line 
    if (token->get_class (token) == TOKEN_EOF
      || token->get_class (token) == TOKEN_EOL) {
      this->priv->errors->set_code
        (this->priv->errors, E_INVALID_PRINT_OUTPUT, line,
        this->priv->last_label);
      statement_destroy (statement);
      statement = NULL;
      token->destroy (token);
    }

    // process a literal string 
    else if (token->get_class (token) == TOKEN_STRING) {
      nextoutput = malloc (sizeof (OutputNode));
      nextoutput->class = OUTPUT_STRING;
      nextoutput->output.string = malloc
	(1 + strlen (token->get_content (token)));
      strcpy (nextoutput->output.string, token->get_content (token));
      nextoutput->next = NULL;
      token->destroy (token);
    }

    // attempt to process an expression 
    else {
      this->priv->stored_token = token;
      if ((expression = parse_expression ())) {
        nextoutput = malloc (sizeof (OutputNode));
        nextoutput->class = OUTPUT_EXPRESSION;
        nextoutput->output.expression = expression;
        nextoutput->next = NULL;
      } else {
        this->priv->errors->set_code
          (this->priv->errors, E_INVALID_PRINT_OUTPUT, token->get_line (token),
             this->priv->last_label);
        statement_destroy (statement);
        statement = NULL;
      }
    }

    // add this output item to the statement and look for another 
    if (! this->priv->errors->get_code (this->priv->errors)) {
      if (lastoutput)
        lastoutput->next = nextoutput;
      else
        statement->statement.printn->first = nextoutput;
      lastoutput = nextoutput;
      token = get_token_to_parse ();
    }

  // continue the loop until the statement appears to be finished 
  } while (! this->priv->errors->get_code (this->priv->errors)
    && (token->get_class (token) == TOKEN_COMMA || token->get_class(token) == TOKEN_SEMICOLON));

  // push back the last token and return the assembled statement 
  if (! this->priv->errors->get_code (this->priv->errors))
    this->priv->stored_token = token;
  return statement;
}

//
 * Parse an INPUT statement
 * returns:
 *   StatementNode*          The statement assembled
 
static StatementNode *parse_input_statement (void) {

  // local variables 
  Token *token = NULL; // tokens read as part of the statement 
  StatementNode *statement; // the statement we're building 
  int line; // line containing the INPUT token 
  VariableListNode
    *nextvar = NULL, // the next variable node we're parsing 
    *lastvar = NULL; // the last variable node we parsed 

  // initialise the statement 
  statement = statement_create ();
  statement->class = STATEMENT_INPUT;
  statement->statement.inputn = statement_create_input ();
  line = this->priv->stream->get_line (this->priv->stream);

  // main loop for parsing the variable list 
  do {

    // discard a previous comma, and seek the next variable 
    if (token) token->destroy (token);
    token = get_token_to_parse ();

    // process a premature end of line 
    if (token->get_class (token) == TOKEN_EOF
      || token->get_line (token) != line) {
      this->priv->errors->set_code
        (this->priv->errors, E_INVALID_VARIABLE, line, this->priv->last_label);
      statement_destroy (statement);
      statement = NULL;
    }

    // attempt to process an variable name 
    else if (token->get_class (token) != TOKEN_VARIABLE) {
      this->priv->errors->set_code
        (this->priv->errors, E_INVALID_VARIABLE, token->get_line (token),
	this->priv->last_label);
      statement_destroy (statement);
      statement = NULL;
    } else {
      nextvar = malloc (sizeof (VariableListNode));
      nextvar->variable = *token->get_content (token) & 0x1f;
      nextvar->next = NULL;
      token->destroy (token);
    }

    // add this variable to the statement and look for another 
    if (! this->priv->errors->get_code (this->priv->errors)) {
      if (lastvar)
        lastvar->next = nextvar;
      else
        statement->statement.inputn->first = nextvar;
      lastvar = nextvar;
      token = get_token_to_parse ();
    }
  } while (! this->priv->errors->get_code (this->priv->errors)
    && token->get_class (token) == TOKEN_COMMA);

  // return the assembled statement 
  this->priv->stored_token = token;
  return statement;
}

//RND function call is an expression, and I'm dumb
// static StatementNode *parse_rnd_statement(void)
{

  // TODO implement
} 

//
 * Parse a statement from the source file
 * returns:
 *   StatementNode*   a fully-assembled statement, hopefully.
 
static StatementNode *parse_statement () {

  // local variables 
  Token *token; // token read 
  StatementNode *statement = NULL; // the new statement 

  // get the next token 
  token = get_token_to_parse ();

  // check for command 
  switch (token->get_class (token)) {
    case TOKEN_EOL:
      this->priv->stored_token = token;
      statement = NULL;
      break;
    case TOKEN_LET:
      token->destroy (token);
      statement = parse_let_statement ();
      break;
    case TOKEN_IF:
      token->destroy (token);
      statement = parse_if_statement ();
      break;
    case TOKEN_GOTO:
      token->destroy (token);
      statement = parse_goto_statement ();
      break;
    case TOKEN_GOSUB:
      token->destroy (token);
      statement = parse_gosub_statement ();
      break;
    case TOKEN_RETURN:
      token->destroy (token);
      statement = parse_return_statement ();
      break;
    case TOKEN_END:
      token->destroy (token);
      statement = parse_end_statement ();
      break;
    case TOKEN_PRINT:
      token->destroy (token);
      statement = parse_print_statement ();
      break;
    case TOKEN_INPUT:
      token->destroy (token);
      statement = parse_input_statement ();
      break;
//     case TOKEN_RND:
      token->destroy (token);
      statement = parse_rnd_statement ();
      break; 
    default:
      this->priv->errors->set_code
        (this->priv->errors, E_UNRECOGNISED_COMMAND, token->get_line (token),
        this->priv->last_label);
      token->destroy (token);
  }

  // return the statement 
  return statement;
}

//
 * Parse a line from the source file.
 * returns:
 *   StatementNode           a fully-assembled statement, hopefully.
 
static ProgramLineNode *parse_program_line (void) {

  // local variables 
  Token *token; // token read 
  ProgramLineNode *program_line; // program line read 
  int label_encountered = 0; // 1 if this line has an explicit label 

  // initialise the program line and get the first token 
  program_line = program_line_create ();
  program_line->label = generate_default_label ();
  token = get_token_to_parse ();

  // deal with end of file 
  if (token->get_class (token) == TOKEN_EOF) {
    token->destroy (token);
    program_line_destroy (program_line);
    return NULL;
  }

  // deal with line label, if supplied 
  if (token->get_class (token) == TOKEN_NUMBER) {
    program_line->label = atoi (token->get_content (token));
    label_encountered = 1;
    token->destroy (token);
  } else
    this->priv->stored_token = token;

  // validate the supplied or implied line label 
  if (! validate_line_label (program_line->label)) {
    this->priv->errors->set_code
      (this->priv->errors, E_INVALID_LINE_NUMBER, this->priv->current_line,
      program_line->label);
    program_line_destroy (program_line);
    return NULL;
  }
  if (label_encountered)
    this->priv->last_label = program_line->label;

  // check for a statement and an EOL 
  program_line->statement = parse_statement ();
  if (! this->priv->errors->get_code (this->priv->errors)) {
    token = get_token_to_parse ();
    if (token->get_class (token) != TOKEN_EOL
      && token->get_class (token) != TOKEN_EOF)
      this->priv->errors->set_code
        (this->priv->errors, E_UNEXPECTED_PARAMETER, this->priv->current_line,
        this->priv->last_label);
    token->destroy (token);
  }
  if (program_line->statement)
    this->priv->last_label = program_line->label;

  // return the program line 
  return program_line;
}


//
 * Public Methods
 


//
 * Parse the whole program
 * params:
 *   Parser*   parser   The parser to use
 * returns:
 *   ProgramNode*       The parsed program
 
static ProgramNode *parse (Parser *parser) {

  // local varables 
  ProgramNode *program; // the stored program 
  ProgramLineNode
    *previous = NULL, // the previous line 
    *current; // the current line 

  // initialise the program 
  this = parser;
  program = malloc (sizeof (ProgramNode));
  program->first = NULL;

  // read lines until reaching an error or end of input 
  while ((current = parse_program_line ())
    && ! this->priv->errors->get_code (this->priv->errors)) {
    if (previous)
      previous->next = current;
    else
      program->first = current;
    previous = current;
  }

  // return the program 
  return program;
}

//
 * Return the current source line we're parsing
 * params:
 *   Parser*   The parser to use
 * returns:
 *   int       the line returned
 
static int get_line (Parser *parser) {
  return parser->priv->current_line;
}

//
 * Return the label of the source line we're parsing
 * params:
 *   Parser*   parser   The parser to use
 * returns:
 *   int                the label returned
 
static int get_label (Parser *parser) {
  return parser->priv->last_label;
}

//
 * Destroy this parser object
 * params:
 *   Parser*   parser   the doomed parser
 
void destroy (Parser *parser) {
  if (parser) {
    if (parser->priv) {
      if (parser->priv->stream)
        parser->priv->stream->destroy (parser->priv->stream);
    }
    free (parser->priv);
  }
  free (parser);
}


//
 * Constructors
 


//
 * Constructor
 * params:
 *   ErrorHandler*      errors    the error handler to use
 *   LanguageOptions*   options   the language options to use
 *   FILE*              input     the input file
 * returns:
 *   Parser*                      the new parser
 
Parser *new_Parser (ErrorHandler *errors, LanguageOptions *options,
  FILE *input) {

  // allocate memory 
  this = malloc (sizeof (Parser));
  this->priv = malloc (sizeof (ParserData));
  this->priv->stream = new_TokenStream (input);

  // initialise methods 
  this->parse = parse;
  this->get_line = get_line;
  this->get_label = get_label;
  this->destroy = destroy;

  // initialise properties 
  this->priv->last_label = 0;
  this->priv->current_line = 0;
  this->priv->end_of_file = 0;
  this->priv->stored_token = NULL;
  this->priv->errors = errors;
  this->priv->options = options;

  // return the new object 
  return this;
}