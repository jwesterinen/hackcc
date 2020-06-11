/*
 * mem.h -- definitions for memory allocation functions
 */

// offsets
extern int g_offset, l_offset, l_max;

void all_program();
void all_var(struct Symtab *symbol, int size, int refLevel);
void all_parm(struct Symtab *symbol);
void all_func(struct Symtab *symbol);

