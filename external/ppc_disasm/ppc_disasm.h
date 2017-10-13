//
// simplified interface to debugger
//

#include "general.h"


int disassemble (char *opcodeStr, char *operandStr, __u32 opcode, __u32 curInstAddr, __u32 *nextInstAddr);

//
// returned opcode types (for high-lighting etc)
//

#define BRANCH_OPCODE       1
#define LDST_OPCODE         2

/*
 *  Usage example 1 (simple) :
 *  --------------------------
 *
 *   char buf[64], opStr[16], parmStr[32];
 *   u32 target;
 *
 *   GekkoDisassemble(opStr, parmStr, opcode, PC, &target);
 *   sprintf(buf, "%-10s %s", opStr, parmStr);    
 *
 *   printf("%.8X  %.8X  %s\n", PC, opcode, buf);
 *
 *  Usage example 2 (opcode type information) :
 *  -------------------------------------------
 *
 *   type = GekkoDisassemble(opStr, parmStr, opcode, PC, &target);
 *   if(type == BRANCH_OPCODE)
 *   {
 *       printf("%-12s%s", opStr, parmStr);
 *       printf(" <-- this is branch to %.8X address\n", target);
 *   }
 *   else ptintf("%-12s%s\n", opStr, parmStr);
 *
 */
