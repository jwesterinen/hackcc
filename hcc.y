/*
 *	hcc99 - a C99 compiler grammar for hack
 *	syntax analysis with error recovery
 * 	(s/r conflict: one on ELSE, one on error)
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "symtab.h"
#include "mem.h"
#include "gen.h"
int yylex(void);

#define OFFSET(x)   (((struct Symtab *)x)->s_offset)
#define NAME(x)     (((struct Symtab *)x)->s_name)
#define TYPE(x)     (((struct Symtab *)x)->s_type)
%}

/*
 * parser stack type union
 */
 
%union  {
            struct Symtab *y_sym;   // Identifier
            char *y_str;            // Constant
            int y_num;              // count, type
            int y_lab;              // label
        }

/*
 *	terminal symbols
 */

%token <y_sym> IDENTIFIER
%token <y_str> CONSTANT 
%token STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPE_NAME

%token TYPEDEF EXTERN STATIC AUTO REGISTER INLINE RESTRICT
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token BOOL COMPLEX IMAGINARY
%token STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

/*
 *	typed non-terminal symbols
 */

%type   <y_sym> parameter_declaration parameter_list declarator direct_declarator
%type   <y_num> type_specifier

%start program
%%

primary_expression
	: IDENTIFIER
	| CONSTANT
	| '(' expression ')'
	;

postfix_expression
	: primary_expression
	| postfix_expression '[' expression ']'
	| postfix_expression '(' ')'
	| postfix_expression '(' argument_expression_list ')'
	| postfix_expression PTR_OP IDENTIFIER
	| postfix_expression INC_OP
	| postfix_expression DEC_OP
	;

argument_expression_list
	: assignment_expression
	| argument_expression_list ',' assignment_expression
	;

unary_expression
	: postfix_expression
	| INC_OP unary_expression
	| DEC_OP unary_expression
	| unary_operator unary_expression
	;

unary_operator
	: '&'
	| '*'
	| '+'
	| '-'
	| '~'
	| '!'
	;

multiplicative_expression
	: unary_expression
	| multiplicative_expression '*' unary_expression
	| multiplicative_expression '/' unary_expression
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression
	| additive_expression '-' multiplicative_expression
	;

shift_expression
	: additive_expression
	| shift_expression LEFT_OP additive_expression
	| shift_expression RIGHT_OP additive_expression
	;

relational_expression
	: shift_expression
	| relational_expression '<' shift_expression
	| relational_expression '>' shift_expression
	| relational_expression LE_OP shift_expression
	| relational_expression GE_OP shift_expression
	;

equality_expression
	: relational_expression
	| equality_expression EQ_OP relational_expression
	| equality_expression NE_OP relational_expression
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression
	;

logical_and_expression
	: exclusive_or_expression
	| logical_and_expression AND_OP exclusive_or_expression
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression OR_OP logical_and_expression
	;

assignment_expression
	: logical_or_expression
	| unary_expression assignment_operator assignment_expression
	;

assignment_operator
	: '='
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LEFT_ASSIGN
	| RIGHT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	;

expression
	: assignment_expression
	| expression ',' 
	    {
	        gen_pr(OP_POP, "discard");
	    }
	  assignment_expression
	;


declaration
	: declaration_specifiers ';'
	| declaration_specifiers init_declarator_list ';'
	;

declaration_specifiers
	: storage_class_specifier
	| storage_class_specifier declaration_specifiers
	| type_specifier
	| type_specifier declaration_specifiers
	;

init_declarator_list
	: init_declarator
	| init_declarator_list ',' init_declarator
	;

init_declarator
	: declarator
	    {
	        all_var($1, 0, 0);
	    }
	| declarator '=' initializer
	    {
	        all_var($1, 0, 0);
	        gen_direct(OP_STORE, gen_mod($1), OFFSET($1), NAME($1));
	        gen_pr(OP_POP, "clear stack");
	    }
	;

storage_class_specifier
	: STATIC
	;

type_specifier
	: VOID
	    {$$=1;}
	| INT
	    {$$=0;}
	;

declarator
	: pointer direct_declarator
	    {
	        set_var_ref_level($2, $<y_num>1);
	        $$ = $2;
	    }
	| direct_declarator
	;

direct_declarator
	: IDENTIFIER
	| '(' declarator ')'
	    {
	        $$ = $2;
	    }
	| direct_declarator '[' CONSTANT ']'
        {
            all_var($1, atoi($3), 0);
        }
	| direct_declarator '[' ']'
	| direct_declarator '(' 
	    {
	        make_func($1);
	        blk_push();
	    }
	  parameter_list ')'
	    {
	        chk_parm($1, parm_default($4));
	        all_parm($4);
	    }
	| direct_declarator '(' ')'
	    {
	        make_func($1);
	        blk_push();
	    }
	;

pointer
	: '*'
        {
            $<y_num>$ = 1;
        }
	| '*' pointer
        {
            $<y_num>$++;
        }
	;

parameter_list
	: parameter_declaration
	    {
	        $$ = link_parm($1, 0);
	        make_parm($1);
	    }
	| parameter_list ',' parameter_declaration
	    {
	        $$ = link_parm($3, $1);
	        make_parm($3);
	    }
	;

parameter_declaration
	: type_specifier declarator
	    {
	        /* could call a semantic check here to ensure all params are ints */
	        $$ = $2;
	    }
	| type_specifier
	    {
	        // this is an annonomous fct prototype so somehow only count the paramters
	        $$ = NULL;
	    }
	;

initializer
	: assignment_expression
	| '{' initializer_list '}'
	;

initializer_list
	: initializer
	| initializer_list ',' initializer
	;



statement
	: labeled_statement
	| compound_statement
	| expression_statement
	| selection_statement
	| iteration_statement
	| jump_statement
	;

labeled_statement
	: IDENTIFIER ':' statement
	| CASE logical_or_expression ':' statement
	| DEFAULT ':' statement
	;

compound_statement
	: '{' '}'
	| '{' 
	    {
	        $<y_lab>$ = l_offset;
	        blk_push();
	    }
	  block_item_list '}'
	    {
	        if (l_offset > l_max)
	            l_max = l_offset;
	        l_offset = $<y_lab>2;
	        blk_pop();
	    }
	;

block_item_list
	: block_item
	| block_item_list block_item
	;

block_item
	: declaration
	| statement
	;

expression_statement
	: ';'
	| expression ';'
	;

selection_statement
	: IF '(' expression ')' statement
	| IF '(' expression ')' statement ELSE statement
	| SWITCH '(' expression ')' statement
	;

iteration_statement
	: WHILE '(' expression ')' statement
	;

jump_statement
	: GOTO IDENTIFIER ';'
	| CONTINUE ';'
	| BREAK ';'
	| RETURN ';'
	| RETURN expression ';'
	;


program
	:   
	    {
	        init();
	        gen_begin_prog();
	    }
	  translation_unit
	    {
	        blk_pop();
	        end_program();
	    }
	;

translation_unit
	: external_declaration
	| translation_unit external_declaration
	;

external_declaration
	: function_definition
	| declaration
	;

function_definition
	: declaration_specifiers declarator 
	    {
	        l_max = 0;
	        $<y_lab>$ = gen_entry($2);
	        set_func_type($2, $<y_num>1);
	    }
	  compound_statement
	    {
	        all_func($2);
	        gen_pr(OP_RETURN, "end of function");
	        fix_entry($2, $<y_lab>3);
	    }
	;


