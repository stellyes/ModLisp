 %{
    #include "cilisp.h"
    #define ylog(r, p) {fprintf(flex_bison_log_file, "BISON: %s ::= %s \n", #r, #p); fflush(flex_bison_log_file);}
    int yylex();
    void yyerror(char*, ...);
%}

%union {
    double dval;
    int ival;
    char* sval;
    char* tval;
    struct ast_node *astNode;
};

%token <ival> FUNC
%token <dval> INT DOUBLE
%token <sval> SYMBOL
%token <tval> TYPE
%token QUIT EOL EOFT LPAREN RPAREN LET COND

%type <astNode> s_expr s_expr_section s_expr_list f_expr let_section let_list let_elem number

%%

program:
    s_expr EOL {
        ylog(program, s_expr EOL);
        if ($1) {
            printRetVal(eval($1));
            freeNode($1);
        }
        YYACCEPT;
    }
    | s_expr EOFT {
        ylog(program, s_expr EOFT);
        if ($1) {
            printRetVal(eval($1));
            freeNode($1);
        }
        exit(EXIT_SUCCESS);
    }
    | EOL {
        ylog(program, EOL);
        YYACCEPT;  // paranoic; main skips blank lines
    }
    | EOFT {
        ylog(program, EOFT);
        exit(EXIT_SUCCESS);
    };


s_expr:
    QUIT {
        ylog(s_expr, QUIT);
        exit(EXIT_SUCCESS);
    }
    | number {
        ylog(s_expr, number);
        $$ = $1;
    }
    | f_expr {
        ylog(s_expr, f_expr);
        $$ = $1;
    }
    | LPAREN let_section s_expr RPAREN
    {
        ylog(s_expr, let_section s_expr);
        $$ = createScopeNode($2, $3);
    }
    | LPAREN COND s_expr s_expr s_expr RPAREN
    {
        ylog(s_expr, COND s_expr s_expr s_expr);
        $$ = createCondNode($3, $4, $5);
    }
    | SYMBOL
    {
        ylog(s_expr, symbol);
        $$ = createSymbolNode_U($1);
    }
    | error {
        ylog(s_expr, error);
        yyerror("unexpected token");
        $$ = NULL;
    };

let_section:
    LPAREN LET let_list RPAREN
    {
        ylog(let_section, let_list);
        $$ = $3;
    };

let_list:
    let_elem
    {
        ylog(let_list, let_elem);
        $$ = $1;
    }
    | let_elem let_list
    {
        ylog(let_list, let_elem let_list);
        $$ = storeSymbolTableNode($1, $2);
    };

let_elem:
    LPAREN SYMBOL s_expr RPAREN
    {
        ylog(let_elem, SYMBOL s_expr);
        $$ = createSymbolNode_I($2, $3);
    }
    | LPAREN TYPE SYMBOL s_expr RPAREN
    {
        ylog(let_elem, TYPE SYMBOL s_expr);
        $$ = createSymbolNode_T($2, $3, $4);
    };

f_expr:
    LPAREN FUNC s_expr_section RPAREN
    {
        ylog(f_expr, s_expr_section);
        $$ = createFunctionNode($2, $3);
    };


s_expr_section:
    s_expr_list
    {
        ylog(s_expr_section, s_expr_list);
        $$ = $1;
    }
    |
    {
        ylog(s_expr_section, <empty>);
        $$ = NULL;
    };


s_expr_list:
    s_expr
    {
       ylog(s_expr_list, s_expr);
       $$ = $1;
    }
    | s_expr s_expr_list
    {
        ylog(s_expr_list, s_expr s_expr_list);
        $$ = addExpressionToList($1, $2);
    }
    | error
    {
        ylog(s_expr_section, error);
        yyerror("unexpected token");
        $$ = NULL;
    };

number:
    INT
    {
        ylog(number, INT);
        $$ = createNumberNode($1, INT_TYPE);
    }
    | DOUBLE
    {
        ylog(number, DOUBLE);
        $$ = createNumberNode($1, DOUBLE_TYPE);
    };

%%
