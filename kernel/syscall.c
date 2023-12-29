/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"

#include "spike_interface/spike_utils.h"
#include "elf.h"
//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  sprint(buf);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

// uint32 get_jal(uint64* ra_addr){
//     uint64 jal_addr = *ra_addr - 4;
//     uint64 get_jal = ROUNDDOWN(jal_addr,8);
//     uint64 delta = jal_addr - get_jal;
//     uint64 first_half, second_half;
//     uint32 result = 0;
//     if(delta == 0){
//         result =  (uint32)*(uint64*)(jal_addr);
//     } else if ( delta <= 4) {
//         uint64 first_half = *(uint64*)(get_jal);
//         first_half =  first_half >> (delta * 8);
//         result = (uint32)first_half;
//         return result;
//     } else {
//         first_half = *(uint64* )get_jal;
//         second_half = *(uint64 *)(get_jal - 8);
//         result = (uint32)(first_half << ((8-delta) * 8) & (second_half >> (delta * 8)));
//     }
//     return result;
// }

extern func_info this_func_info[32];
extern uint64 func_num;

int query_func_name(uint64 ra)
{
    for(uint64 i = 0; i < func_num; i++){
        if(ra > this_func_info[i].st_value && 
           ra < this_func_info[i].st_value + this_func_info[i].st_size){
          sprint("%s\n",this_func_info[i].func_name);
          return 1;    
        }
    }
    return 0;
}


//
// TODO lab1-challenge1 打印用户程序调用栈
// implement sys_user_print_backtrace
//
ssize_t sys_user_print_backtrace(uint64 depth) {
    uint64 trace_depth = 0;
    uint64* fp_addr = (uint64 *)(current->trapframe->regs.s0 - 8);
    uint64* ra_addr = (uint64 *)(*fp_addr - 8);
    // uint64 test_addr = 2164261008;
    // sprint("test: %x\n", *(uint64*)test_addr);
    while(*ra_addr != 0 && trace_depth != depth){
        // sprint("this_ra : <%x>\n",*ra_addr);
        if(!query_func_name(*ra_addr)){
          panic("query function name error");
          return -1;
        }
        fp_addr = (uint64 *)(*fp_addr - 16);
        ra_addr = (uint64 *)(*fp_addr - 8);
        // uint64 jal_content = get_jal(ra_addr);
        // sprint("jal : %x\n",jal_content);
        trace_depth++;
    }
    
    return trace_depth;
}




//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    case SYS_user_print_backtrace: // TODO lab1-challenge1 打印用户程序调用栈
      return sys_user_print_backtrace(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
