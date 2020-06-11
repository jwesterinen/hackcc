/*
 *  mem.c -- memory allocation functions
 */

#include <stdlib.h>
#include <string.h>
#include "y.tab.h"
#include "symtab.h"
#include "message.h"

// offsets
int g_offset = 1,           // offset in global region
    l_offset = 0,           // offset in local region
    l_max;                  // size of local region

// complete all allocations
void all_program()
{
    blk_pop();
#ifdef TRACE
    message("global region has %d word(s)", g_offset);
#endif
}

// generate variable offsets
void all_var(struct Symtab *symbol, int size, int refLevel)
{
    symbol = make_var(symbol, size, refLevel);
    
    // if not in parameter region assign sutable offsets
    switch (symbol->s_blknum)
    {
        default:
            //symbol->s_offset = l_offset++;
            symbol->s_offset = l_offset;
            l_offset += (symbol->s_size == 0) ? 1 : symbol->s_size;
        case 2:
            break;
        case 1:
            //symbol->s_offset = g_offset++;
            symbol->s_offset = g_offset;
            g_offset += (symbol->s_size == 0) ? 1 : symbol->s_size;
            break;
        case 0:
            bug("all_var");
    }
}

// generate parameter offsets
void all_parm(struct Symtab *symbol)
{
    int p_offset = 0;
    
    while (symbol)
    {
        symbol->s_offset = p_offset++;
        symbol = symbol->s_plist;
    }
#ifdef TRACE
    message("parameter region has %d word(s)", p_offset);
#endif
}

// complete the allocation of a function
void all_func(struct Symtab *symbol)
{
    blk_pop();
#ifdef TRACE
    message("local region has %d word(s)", l_max);
#endif
}


// end of mem.c

