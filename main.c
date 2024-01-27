#include <stdio.h>
#include "stdbool.h"
#include <stdlib.h>
#include <math.h>

#define PREC_MIN -999
#define MAX_INT_PARTS = 128;

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

    bool                negative;
    unsigned long long  integer_part;
    long long           exp;
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
struct token_t  statement_execute(struct node_t *node);
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
            fprintf(stdout, "\x1b[35mINVALID\x1b[0m");
            break;
        case TOK_LITERAL_INTEGER_NOSIZE:
            fprintf(stdout, "\x1b[34m{NUM: \x1b[0m");
            if (token.integer_part == 0)
            {
                putc('0', stdout);
            }
            else
            {
                if (token.negative)
                {
                    putc('-', stdout);
                }

                char buffer[100];
                buffer[99] = '\0';
                int i = 98;
                long long exp = token.exp;
                unsigned long long disp = token.integer_part;

                if (token.exp < 0)
                {
                    for (; i >= 0 && disp > 0; i--)
                    {
                        if (exp == 0) {
                            buffer[i] = '.';
                            exp++;
                            continue;
                        }
                        int x = disp % 10;
                        buffer[i] = x + '0';
                        disp /= 10;
                        exp++;
                    }
                    for (; i >= 0 && exp < 0; i--)
                    {
                        buffer[i] = '0';
                        exp++;
                    }
                    if (i == 0)
                    {
                        buffer[i--] = '.';
                        buffer[i--] = '0';
                    }
                }
                else
                {
                    int comma = 0;

                    for (; i >= 0 && exp > 0; i--)
                    {
                        if (comma == 3)
                        {
                            buffer[i] = ',';
                            comma = 0;
                            continue;
                        }

                        buffer[i] = '0';
                        exp--;
                        comma++;
                    }

                    for (; i >= 0 && disp > 0; i--)
                    {
                        if (comma == 3)
                        {
                            buffer[i] = ',';
                            comma = 0;
                            continue;
                        }

                        int x = disp % 10;
                        buffer[i] = x + '0';
                        disp /= 10;
                        comma++;
                    }
                }
                fprintf(stdout, "%s", buffer+i+1);
            }
            fprintf(stdout, "\x1b[34m}\x1b[0m");
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

struct token_t statement_execute(struct node_t *node)
{
    if (token_is_binary(node->token))
    {
        struct token_t left = statement_execute(node->left);
        struct token_t right = statement_execute(node->right);

        if (left.kind != TOK_LITERAL_INTEGER_NOSIZE ||
            right.kind != TOK_LITERAL_INTEGER_NOSIZE)
        {
            return (struct token_t){ .kind = TOK_INVALID };
        }
    
        if (node->token.kind == TOK_MINUS)
        {
            right.negative = !right.negative;
        }

        switch (node->token.kind)
        {
            case TOK_PLUS:
            case TOK_MINUS:

                // 1e3 + 1e0
                //           = 1000 + 1
                //           = 1001 
                //           = 1001e0


                // 1e3 + 1e-2
                //           = 1000 + 0.01
                //           = 1000.01 
                //           = 100001e-2

                // 1e3 - 1e0
                //           = 1000 - 1
                //           =  999 
                //           =  999e0


                // 1e3 - 1e-2
                //           = 1000 - 0.01
                //           =  999.99 
                //           =  99999e-2

                // TODO JOSH: Safety
                long long expmin, expmax;
                if (left.exp > right.exp)
                {
                    expmin = right.exp;
                    expmax = left.exp;

                    for (int p = expmax - expmin; p > 0; p--)
                    {
                        left.integer_part *= 10;
                    }
                }
                else
                {
                    expmin = left.exp;
                    expmax = right.exp;

                    for (int p = expmax - expmin; p > 0; p--)
                    {
                        right.integer_part *= 10;
                    }
                }

                left.exp = expmin;
                if (left.negative == right.negative)
                {
                    left.integer_part += right.integer_part;
                }
                else
                {
                    if (left.integer_part > right.integer_part)
                    {
                        left.integer_part -= right.integer_part;
                    }
                    else
                    {
                        left.integer_part = right.integer_part - left.integer_part;
                    }
                }
                break;
            // case TOK_MULTIPLY:
            //     break;
            // case TOK_DIVIDE:
            //     break;

            default: 
                return (struct token_t){ .kind = TOK_INVALID };
        }

        return left;
    }
    
    switch (node->token.kind)
    {
        case TOK_LITERAL_INTEGER_NOSIZE: return node->token;

        // case TOK_PLUS:     return statement_long_val(node->left) + statement_long_val(node->right);
        // case TOK_MINUS:    return statement_long_val(node->left) - statement_long_val(node->right);
        // case TOK_MULTIPLY: return statement_long_val(node->left) * statement_long_val(node->right);
        // case TOK_DIVIDE:   return statement_long_val(node->left) / statement_long_val(node->right);

        default: 
            return (struct token_t){ .kind = TOK_INVALID };
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
        long long exp = 0;
        unsigned long long integer_part = c - '0';

        while (class_is_numeric(char_peek())) {
            c = char_next();
            integer_part *= 10;
            integer_part += c - '0';
        }

        if (char_peek() == '.') 
        {
            char_next();

            while (class_is_numeric(char_peek())) {
                c = char_next();
                integer_part *= 10;
                integer_part += c - '0';
                exp--;
            }
        }

        if (char_peek() == 'E' ||
            char_peek() == 'e')
        {
            char_next();

            bool exp_negative = false;
            if (char_peek() == '-')
            {
                char_next();
                exp_negative = true;
            }

            // TODO JOSH: Error if non-numeric

            long long exp_given = 0;
            while (class_is_numeric(char_peek())) {
                c = char_next();
                exp_given *= 10;
                exp_given += c - '0';
            }

            if (exp_negative) exp_given *= -1;

            exp += exp_given;
        }

        return (struct token_t)
        {
            .kind           = TOK_LITERAL_INTEGER_NOSIZE,
            .negative       = negative,
            .integer_part   = integer_part,
            .exp            = exp,
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
        // display_node(n);
        // putc('\n', stdout);

        struct token_t result = statement_execute(n);
        fprintf(stdout, "  = ");
        display_token(result);
        putc('\n', stdout);

        nodes_size = 0;
        _peek_char = 0;
        _peek_token.kind = TOK_PEEK_EMPTY;
    }
}
