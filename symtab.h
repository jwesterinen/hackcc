/*
 * symtab.h -- definitions for symbol table
 */

struct Symtab
{
    char   *s_name;             // name
    int     s_type;             // symbol type
    int     s_blknum;           // static block depth
    union 
    {
        int s__num;
        struct Symtab *s__link;
    } s__; 
    int     s_offset;           // symbol definition   
    int     s_size;             // size of the variable, 1 for normal vars, n for arrays
    int     s_is_void;          // flag to specifiy a void function
    int     s_ref_level;        // reference level, 0 for scalars, +1 for each pointer level
    struct Symtab *s_next;      // next entry
};

#define s_pnum  s__.s__num      // count of parameters
#define NOT_SET (-1)            // no count yet set
#define s_plist s__.s__link     // chain of parameters

// s_type values
#define UDEC    0               // not declared
#define FUNC    1               // function
#define UFUNC   2               // undefined function
#define VAR     3               // declared variable
#define PARM    4               // undeclared parameter

// s_type string values
#define SYMmap   "udecl", "fct", "udef fct", "var", "parm"

// reference level
extern int ref_level;

void init();
struct Symtab *s_find(const char *name);
void s_lookup(int yylex);

void blk_push();
void blk_pop();

struct Symtab *make_parm(struct Symtab *symbol);
struct Symtab *link_parm(struct Symtab *symbol, struct Symtab *next);
void chk_parm(struct Symtab *symbol, int count);
int parm_default(struct Symtab *symbol);

struct Symtab *make_var(struct Symtab *symbol, int size, int refLevel);
void set_var_ref_level(struct Symtab *symbol, int refLevel);
int chk_var(struct Symtab *symbol);

struct Symtab *make_func(struct Symtab *symbol);
void chk_func(struct Symtab *symbol);
void set_func_type(struct Symtab *symbol, int isVoid);

