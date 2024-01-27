#include <stdio.h>
#include "stdbool.h"
#include <stdlib.h>

#define PREC_MIN -999

enum token_kind_t {
    // Special
    TOK_EOF        = '\x03',
    TOK_INVALID    = '\x07',
    TOK_PEEK_EMPTY = 'P',

    // Variable-length tokens
    TOK_LITERAL_INTEGER_NOSIZE = '1',
    TOK_LITERAL_STRING         = 's',

    // Binary Operators
    TOK_PLUS     = '+',
    TOK_MINUS    = '-',
    TOK_MULTIPLY = '*',
    TOK_DIVIDE   = '/',
};

struct token_t {
    enum token_kind_t kind;

    union {
        long        val_long;
        int         val_int;
        const char *val_string;
    };
};

struct node_t {
    struct node_t  *left;
    struct node_t  *right;
    struct token_t token;
};

int             char_next();
int             char_peek();
bool            class_is_numeric(int c);
bool            class_is_numeric(int c);
bool            class_is_ws(int c);
void            display_token(struct token_t token);
void            display_node(struct node_t *node);
struct node_t*  node_make();
struct node_t*  parse_binaryexp(struct node_t *left, int minprec);
struct node_t*  parse_expression(int minprec);
struct node_t*  parse_leaf();
void            statement_execute(struct node_t *node);
long            statement_long_val(struct node_t *node);
bool            token_is_binary(struct token_t token);
int             token_get_prec(struct token_t token);
struct token_t  token_next();
struct token_t  token_peek();

struct node_t  *nodes_all;
size_t          nodes_size;
size_t          nodes_cap;

int _peek_char;
int char_next() {
    int c;
    if (_peek_char) {
        c = _peek_char;
        _peek_char = 0;
    }
    else {
        c = getc(stdin);
    }
    return c;
}

int char_peek() {
    if (!_peek_char) {
        _peek_char = getc(stdin);
    }
    return _peek_char;
}

bool class_is_numeric(int c) {
    return c >= '0' && c <= '9';
}

bool class_is_ws(int c) {
    return c == ' ' || c == '\t';
}

void display_token(struct token_t token) {
    switch (token.kind) {
        case TOK_INVALID:
            fprintf(stdout, "\x1b[35m??\x1b[0m");
            break;
        case TOK_LITERAL_INTEGER_NOSIZE:
            fprintf(stdout, "\x1b[34m{NUM: %li}\x1b[0m", token.val_long);
            break;
        default:
            putc(token.kind, stdout);
            break;
    }
}

void display_node(struct node_t *node) 
{
    if (node->left)
    {
        putc('(', stdout);
        putc(' ', stdout);
        display_node(node->left);
        putc(' ', stdout);
        display_token(node->token);
        putc(' ', stdout);
        if (node->right)
        {
            display_node(node->right);
            putc(' ', stdout);
        }
        putc(')', stdout);
    }
    else
    {
        display_token(node->token);
    }
}

struct node_t* node_make() {
    if (nodes_size == nodes_cap) {
        fprintf(stderr, "Exceeded max node capacity\n");
        exit(1);
    }

    struct node_t* node = nodes_all + nodes_size;
    nodes_size++;

    node->left = NULL;
    node->right = NULL;
    node->token.kind = TOK_INVALID;

    return node;
}

struct node_t* parse_binaryexp(struct node_t *left, int minprec)
{
    struct token_t op = token_peek();
    if (!token_is_binary(op)) 
    {
        return left;
    }

    int prec = token_get_prec(op);

    if (prec <= minprec) 
    {
        return left;
    }

    token_next();// consume

    struct node_t *right = parse_expression(prec);

    struct node_t *node = node_make();
    node->left = left;
    node->token = op;
    node->right = right;

    return node;
}

struct node_t* parse_expression(int minprec) {

    struct node_t *left = parse_leaf();
    
    while (true) 
    {
        struct node_t *node = parse_binaryexp(left, minprec);
        if (node == left) {
            break;
        }
        left = node;
    }

    return left;
}

struct node_t* parse_leaf() {
    struct token_t token = token_next();

    // TODO JOSH: Check kind

    struct node_t *node = node_make();
    node->left = NULL;
    node->right = NULL;
    node->token = token;

    return node;
}

void statement_execute(struct node_t *node)
{
    fprintf(stdout, "  = %li\n", statement_long_val(node));
}

long statement_long_val(struct node_t *node)
{
    switch (node->token.kind)
    {
        case TOK_LITERAL_INTEGER_NOSIZE: return node->token.val_long;

        case TOK_PLUS:     return statement_long_val(node->left) + statement_long_val(node->right);
        case TOK_MINUS:    return statement_long_val(node->left) - statement_long_val(node->right);
        case TOK_MULTIPLY: return statement_long_val(node->left) * statement_long_val(node->right);
        case TOK_DIVIDE:   return statement_long_val(node->left) / statement_long_val(node->right);

        default: return 0;
    }
}

bool token_is_binary(struct token_t token) 
{
    switch (token.kind)
    {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_MULTIPLY:
        case TOK_DIVIDE:
            return true;
        default:
            return false;
    }
}

int token_get_prec(struct token_t token)
{
    switch (token.kind)
    {
        case TOK_PLUS:
        case TOK_MINUS:
            return 1;
        case TOK_MULTIPLY:
        case TOK_DIVIDE:
            return 2;
        default:
            return PREC_MIN;
    }
}

struct token_t _peek_token;

struct token_t token_next() {

    if (_peek_token.kind != TOK_PEEK_EMPTY) 
    {
        struct token_t t = _peek_token;
        _peek_token.kind = TOK_PEEK_EMPTY;
        return t;
    }

    int c = char_next();
    while (class_is_ws(c)) {
         c = char_next();
    }

    if (class_is_numeric(c) ||
        (c == '-' && class_is_numeric(char_peek()))) 
    {
        bool negative = false;
        if (c == '-') {
            negative = true;
            c = char_next();
        }
        long val_long = c - '0';

        while (class_is_numeric(char_peek())) {
            c = char_next();
            val_long *= 10;
            val_long += c - '0';
        }

        if (negative) val_long *= -1;

        return (struct token_t)
        {
            .kind     = TOK_LITERAL_INTEGER_NOSIZE,
            .val_long = val_long,
        };
    }

    switch (c) {
        // Special cases
        case EOF:
        case '\r':
        case '\n':
            return (struct token_t){ .kind = TOK_EOF };
        default:  return (struct token_t){ .kind = TOK_INVALID };

        // Binary Operators
        case '+': return (struct token_t){ .kind = TOK_PLUS };
        case '-': return (struct token_t){ .kind = TOK_MINUS };
        case '*': return (struct token_t){ .kind = TOK_MULTIPLY };
        case '/': return (struct token_t){ .kind = TOK_DIVIDE };
    }
}

struct token_t token_peek()
{
    if (_peek_token.kind == TOK_PEEK_EMPTY) 
    {
        _peek_token = token_next();
    }
    return _peek_token;
}

int main() {
    _peek_token.kind = TOK_PEEK_EMPTY;

    nodes_size = 0;
    nodes_cap = 4096;
    nodes_all = malloc(nodes_cap * sizeof(struct node_t));

    while (true)
    {
        fprintf(stdout, "> ");
        struct node_t *n = parse_expression(PREC_MIN);
        display_node(n);
        // putc('\n', stdout);

        statement_execute(n);

        nodes_size = 0;
        _peek_char = 0;
        _peek_token.kind = TOK_PEEK_EMPTY;
    }
}
