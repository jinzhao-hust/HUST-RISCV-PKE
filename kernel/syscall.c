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

//
// TODO lab1-challenge1 打印用户程序调用栈
// implement sys_user_print_backtrace
//
ssize_t sys_user_print_backtrace(uint64 depth) {
    uint64 trace_depth = 0;
    uint64* fp_addr = (uint64 *)(current->trapframe->regs.s0 - 8);
    uint64* ra_addr = (uint64 *)(*fp_addr - 8);
    while(*ra_addr != 0 && trace_depth != depth){
        sprint("this_ra : <%x>\n",*ra_addr);
        fp_addr = (uint64 *)(*fp_addr - 16);
        ra_addr = (uint64 *)(*fp_addr - 8);
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
