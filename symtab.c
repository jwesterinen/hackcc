/*
 *  symtab.c -- symbol table
 */

#include <stdlib.h>
#include <string.h>
#include "y.tab.h"
#include "symtab.h"
#include "mem.h"
#include "message.h"

extern YYSTYPE yylval;

// symbol table
static struct Symtab 
    symtab,                 // blind element
    *s_gbl;                 // global end of chain
#define s_lcl (&symtab)     // local end of chain   

// current static block depth
static int blknum = 0;

// reference level
int ref_level = 0;    
    
static char* strsave(const char* s)
{
    char* cp = calloc(strlen(s)+1, 1);
    if (cp)
    {
        strcpy(cp, s);
        return cp;
    }
    
    return (char*)NULL;
}
  
static struct Symtab *s_create(const char *name)
{
    struct Symtab *newEntry = (struct Symtab *)calloc(1, sizeof(struct Symtab));
    
    if (newEntry)
    {
        newEntry->s_next = s_lcl->s_next;
        s_lcl->s_next = newEntry;
        newEntry->s_name = strsave(name);
        newEntry->s_type = UDEC;
        newEntry->s_blknum = 0;
        newEntry->s_pnum = NOT_SET;
    }
     
    return newEntry;
}

static int s_move(struct Symtab *symbol)
{
    struct Symtab *ptr;
    
    // find desired entry in symtab chain
    for (ptr = s_lcl; ptr->s_next != symbol; ptr = ptr->s_next)
        if (!ptr->s_next)
            return -1;
            
    // unlink it from its present position and relink at global end
    ptr->s_next = symbol->s_next;
    s_gbl->s_next = symbol;
    s_gbl = symbol;
    s_gbl->s_next = (struct Symtab *)NULL;
    
    return 0;       
}

// init the global end of chain to point to the "main" function
void init()
{
    blk_push();
    s_gbl = s_create("main");
    s_gbl->s_type = UFUNC;
}

// push the block stack
void blk_push()
{
    blknum++;
}

// pop the block stack
void blk_pop()
{
    struct Symtab *ptr;
    
    for (ptr = s_lcl->s_next; ptr && (ptr->s_blknum >= blknum || ptr->s_blknum == 0); ptr = s_lcl->s_next)
    {
        if (!ptr->s_name)
            bug("blk_pop NULL name");
#ifdef TRACE
        {
            static char *type[] = {SYMmap}; 
            if (ptr->s_type == FUNC)
            {
                message("Popping %s: %s, depth %d, offset %d, return type %s", 
                        ptr->s_name, type[ptr->s_type], ptr->s_blknum, ptr->s_offset, (ptr->s_is_void) ? "void" : "int");
            }
            else
            {
                message("Popping %s: %s, depth %d, offset %d", 
                        ptr->s_name, type[ptr->s_type], ptr->s_blknum, ptr->s_offset);
            }
        }
#endif 
        if (ptr->s_type == UFUNC)
            warning("undefined function %s", ptr->s_name);
        free(ptr->s_name);
        s_lcl->s_next = ptr->s_next;
        free(ptr);
    }
    blknum--;
}

// symbol table lookup used in lexer
void s_lookup(int yylex)
{
    extern char *yytext;

    switch (yylex)
    {
        case CONSTANT:
            yylval.y_str = strsave(yytext);
            break;
        case IDENTIFIER:
            if ((yylval.y_sym = s_find(yytext)))
                break;
            yylval.y_sym = s_create(yytext);
            break;
        default:
            bug("s_lookup");
    }    
}

struct Symtab *s_find(const char *name)
{
    struct Symtab *ptr;
    
    // search symtab until match of end of symtab chain
    for (ptr = s_lcl->s_next; ptr; ptr = ptr->s_next)
    {
        if (!ptr->s_name)
            bug("s_find");
        else
            if (strcmp(ptr->s_name, name) == 0)
                return ptr;
                
    }
   
    return (struct Symtab *)NULL;
}

struct Symtab *make_var(struct Symtab *symbol, int size, int refLevel)
{
    switch (symbol->s_type)
    {
        case VAR:
        case FUNC:
        case UFUNC:
            if (symbol->s_blknum == blknum ||
                (symbol->s_blknum == 2 && blknum == 3))
                    error("duplicate name %s", symbol->s_name);
            symbol = s_create(symbol->s_name);
        case UDEC:
            break;
        case PARM:
            error("unexpected parameter %s", symbol->s_name);
            break;
        default:
            bug("make_var");
    }
    symbol->s_type = VAR;
    symbol->s_blknum = blknum;
    
    // size = 0 for scalars or = number of elements in an array
    symbol->s_size = size;
    
    // refLevel = 0 for scalars or the number of pointer level deep, e.g. int *x, **y, ****z;
    symbol->s_ref_level = refLevel;

    return symbol;
}

void set_var_ref_level(struct Symtab *symbol, int refLevel)
{
    // refLevel = 0 for scalars or the number of pointer level deep, e.g. int *x, **y, ****z;
    symbol->s_ref_level = refLevel;
}

int chk_var(struct Symtab *symbol)
{
    switch (symbol->s_type)
    {
        case UDEC:
            error("undeclared varible %s", symbol->s_name);
            break;
        case PARM:
            error("unexpected parameter %s", symbol->s_name);
            break;
        case FUNC:
        case UFUNC:
            error("function %s used as variable", symbol->s_name);
        case VAR:
            return symbol->s_size;
        default:
            bug("check_var");
    }
    symbol->s_type = VAR;
    symbol->s_blknum = blknum;
    
    return symbol->s_size;
}


struct Symtab *make_parm(struct Symtab *symbol)
{
    switch (symbol->s_type)
    {
        case VAR:
            if (symbol->s_blknum == 2)
            {
                error("parameter %s declared twice", symbol->s_name);
                return symbol;
            }
        case FUNC:
        case UDEC:
            error("%s is not a parameter", symbol->s_name);
            symbol = s_create(symbol->s_name);
        case PARM:
            break;
        default:
            bug("make_parm");
    }
    symbol->s_type = VAR;
    symbol->s_blknum = blknum;

    return symbol;
}

struct Symtab *link_parm(struct Symtab *symbol, struct Symtab *next)
{
    switch (symbol->s_type)
    {
        case PARM:
            error("duplicate parameter %s", symbol->s_name);
            return next;
        case FUNC:
        case UFUNC:
        case VAR:
            symbol = s_create(symbol->s_name);
        case UDEC:
            break;
        default:
            bug("link_parm");
    }
    symbol->s_type = PARM;
    symbol->s_blknum = blknum;
    symbol->s_plist = next;

    return symbol;
}

void chk_parm(struct Symtab *symbol, int count)
{
    if (symbol->s_pnum == NOT_SET)
        symbol->s_pnum = count;
    else if (symbol->s_pnum != count)
        warning("function %s should have %d arguments(s)", symbol->s_name, symbol->s_pnum);
}

int parm_default(struct Symtab *symbol)
{
    int count = 0;
    
    while (symbol)
    {
        count++;
        if (symbol->s_type == PARM)
            symbol->s_type = VAR;
        symbol = symbol->s_plist;
    }

    return count;        
}

struct Symtab *make_func(struct Symtab *symbol)
{
    switch (symbol->s_type)
    {
        case UFUNC:
        case UDEC:
            break;
        case VAR:
            error("function name %s same as global variable", symbol->s_name);
            return symbol;
        case FUNC:
            error("duplicate function definition %s", symbol->s_name);
            return symbol;
        default:
            bug("make_func");
    }
    symbol->s_type = FUNC;
    symbol->s_blknum = 1;

    return symbol;
}

void chk_func(struct Symtab *symbol)
{
    switch (symbol->s_type)
    {
        case UDEC:
            break;
        case PARM:
            error("unexpected parameter %s", symbol->s_name);
            symbol->s_pnum = NOT_SET;
            return;
        case VAR:
            error("variable %s used as function", symbol->s_name);
            symbol->s_pnum = NOT_SET;
        case FUNC:
        case UFUNC:
            return;
        default:
            bug("check_func");
    }
    s_move(symbol);
    symbol->s_type = UFUNC;
    symbol->s_blknum = 1;
}

void set_func_type(struct Symtab *symbol, int isVoid)
{
    symbol->s_is_void = isVoid;
}

// end of symtab.c

