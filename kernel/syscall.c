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
#include "pmm.h"
#include "vmm.h"
#include "sched.h"
#include "proc_file.h"

#include "spike_interface/spike_utils.h"
//
// added @lab4_challenge1
//
void find_pwd_path(struct dentry* now, char record[]){
//   char tmp[30];
//   if (now->parent == NULL)
//   {
//     if(strlen(record)==0){
//       tmp[0] = '/';
//       tmp[1] = '\0';
//     }
//     else
//     {
//       tmp[0] = '\0';
//     }
//     strcat(tmp, record);
//     strcpy(record, tmp);
//   }
//   else
//   {
//     tmp[0] = '/';
//     tmp[1] = '\0';
//     strcat(tmp, now->name);
//     strcat(tmp, record);
//     strcpy(record, tmp);
//     find_pwd_path(now->parent, record);
//   }
  if(now -> parent == NULL){
    record[0] = '/';
    record[1] = '\0';
    return ;
  } else {
    find_pwd_path(now->parent, record);
    strcat(record, now->name);
  }
}

char* parse_path(char* pathname, char* resolved_path){
  if(pathname[0] == '/'){
    // 绝对路径直接返回
    return pathname;
  }
  // 获取当前目录
  find_pwd_path(current->pfiles->cwd, resolved_path);
  
  size_t path_lengh = strlen(resolved_path);
  if(resolved_path[path_lengh - 1] != '/'){
    strcat(resolved_path,"/");
  }
  // copy
  char path_copy[MAX_PATH_LEN];
  strcpy(path_copy, pathname);
  // 解析相对路径
  char* token = strtok(path_copy, "/");
  while(token != NULL){
    if (strcmp(token,".") == 0) {
      // 继续向后解析
      goto next_part;
    } else if (strcmp(token,"..") == 0) {
      // 到根目录就继续向后解析
      if(strlen(resolved_path) == 1) goto next_part;
      // 去掉尾巴的/
      char* las_seq = strrchr(resolved_path,'/');
      if(las_seq != NULL) *las_seq = '\0';
      // 去掉上级目录
      las_seq = strrchr(resolved_path,'/');
      if(las_seq != NULL) *las_seq = '\0';
      // 加个/
       strcat(resolved_path,"/");
    } else {
      // 文件名就直接添加到后面
      strcat(resolved_path,token);
      // 加个/
      strcat(resolved_path,"/");
    }
    // 继续解析下一部分
    next_part:
    token = strtok(NULL, "/");
  }
  char* las_seq = strrchr(resolved_path,'/');
  if(las_seq) *las_seq = '\0'; 
  //sprint("resolved path : %s\n",resolved_path);
  return resolved_path;
}





//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // reclaim the current process, and reschedule. added @lab3_1
  free_process( current );
  schedule();
  return 0;
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page() {
  void* pa = alloc_page();
  uint64 va;
  // if there are previously reclaimed pages, use them first (this does not change the
  // size of the heap)
  if (current->user_heap.free_pages_count > 0) {
    va =  current->user_heap.free_pages_address[--current->user_heap.free_pages_count];
    assert(va < current->user_heap.heap_top);
  } else {
    // otherwise, allocate a new page (this increases the size of the heap by one page)
    va = current->user_heap.heap_top;
    current->user_heap.heap_top += PGSIZE;

    current->mapped_info[HEAP_SEGMENT].npages++;
  }
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  // add the reclaimed page to the free page list
  current->user_heap.free_pages_address[current->user_heap.free_pages_count++] = va;
  return 0;
}

//
// kerenl entry point of naive_fork
//
ssize_t sys_user_fork() {
  sprint("User call fork.\n");
  return do_fork( current );
}

//
// kerenl entry point of yield. added @lab3_2
//
ssize_t sys_user_yield() {
  // TODO (lab3_2): implment the syscall of yield.
  // hint: the functionality of yield is to give up the processor. therefore,
  // we should set the status of currently running process to READY, insert it in
  // the rear of ready queue, and finally, schedule a READY process to run.
  // panic( "You need to implement the yield syscall in lab3_2.\n" );
  current->status = READY;
  insert_to_ready_queue(current);
  schedule();
  return 0;
}

//
// open file
//
ssize_t sys_user_open(char *pathva, int flags) {
  char* pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  //sprint("sys call open %s \n",pathpa);
  char resolved_path[MAX_PATH_LEN];
  memset(resolved_path,0,MAX_PATH_LEN);
  return do_open(parse_path(pathpa, resolved_path), flags);
}

//
// read file
//
ssize_t sys_user_read(int fd, char *bufva, uint64 count) {
  int i = 0;
  while (i < count) { // count can be greater than page size
    uint64 addr = (uint64)bufva + i;
    uint64 pa = lookup_pa((pagetable_t)current->pagetable, addr);
    uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
    uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
    uint64 r = do_read(fd, (char *)pa + off, len);
    i += r; if (r < len) return i;
  }
  return count;
}

//
// write file
//
ssize_t sys_user_write(int fd, char *bufva, uint64 count) {
  int i = 0;
  while (i < count) { // count can be greater than page size
    uint64 addr = (uint64)bufva + i;
    uint64 pa = lookup_pa((pagetable_t)current->pagetable, addr);
    uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
    uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
    uint64 r = do_write(fd, (char *)pa + off, len);
    i += r; if (r < len) return i;
  }
  return count;
}

//
// lseek file
//
ssize_t sys_user_lseek(int fd, int offset, int whence) {
  return do_lseek(fd, offset, whence);
}

//
// read vinode
//
ssize_t sys_user_stat(int fd, struct istat *istat) {
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current->pagetable), istat);
  return do_stat(fd, pistat);
}

//
// read disk inode
//
ssize_t sys_user_disk_stat(int fd, struct istat *istat) {
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current->pagetable), istat);
  return do_disk_stat(fd, pistat);
}

//
// close file
//
ssize_t sys_user_close(int fd) {
  return do_close(fd);
}

//
// lib call to opendir
//
ssize_t sys_user_opendir(char * pathva){
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  return do_opendir(pathpa);
}

//
// lib call to readdir
//
ssize_t sys_user_readdir(int fd, struct dir *vdir){
  struct dir * pdir = (struct dir *)user_va_to_pa((pagetable_t)(current->pagetable), vdir);
  return do_readdir(fd, pdir);
}

//
// lib call to mkdir
//
ssize_t sys_user_mkdir(char * pathva){
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  return do_mkdir(pathpa);
}

//
// lib call to closedir
//
ssize_t sys_user_closedir(int fd){
  return do_closedir(fd);
}

//
// lib call to link
//
ssize_t sys_user_link(char * vfn1, char * vfn2){
  char * pfn1 = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)vfn1);
  char * pfn2 = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)vfn2);
  return do_link(pfn1, pfn2);
}

//
// lib call to unlink
//
ssize_t sys_user_unlink(char * vfn){
  char * pfn = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)vfn);
  return do_unlink(pfn);
}

//
// lib call to rcwd
//
ssize_t sys_user_rcwd(char* pathva){
  // sprint("sys call rcwd\n");
  char* pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable),(void*)pathva);
  memset(pathpa, 0, MAX_PATH_LEN);
  find_pwd_path(current->pfiles->cwd,pathpa);
  return 0;
}
//
// lib call to rcwd
// 
ssize_t sys_user_ccwd(const char* pathva){
  char* pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable),(void*)pathva);
  char new_path[MAX_DEVICE_NAME_LEN];
  memset(new_path, '\0', MAX_DENTRY_NAME_LEN);
  struct dentry *current_directory = current->pfiles->cwd;
  parse_path(pathpa,new_path);
  int fd = do_opendir(new_path);
  current->pfiles->cwd = current->pfiles->opened_files[fd].f_dentry;
  do_closedir(fd);
  return 0;
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
    // added @lab2_2
    case SYS_user_allocate_page:
      return sys_user_allocate_page();
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    case SYS_user_fork:
      return sys_user_fork();
    case SYS_user_yield:
      return sys_user_yield();
    // added @lab4_1
    case SYS_user_open:
      return sys_user_open((char *)a1, a2);
    case SYS_user_read:
      return sys_user_read(a1, (char *)a2, a3);
    case SYS_user_write:
      return sys_user_write(a1, (char *)a2, a3);
    case SYS_user_lseek:
      return sys_user_lseek(a1, a2, a3);
    case SYS_user_stat:
      return sys_user_stat(a1, (struct istat *)a2);
    case SYS_user_disk_stat:
      return sys_user_disk_stat(a1, (struct istat *)a2);
    case SYS_user_close:
      return sys_user_close(a1);
    // added @lab4_2
    case SYS_user_opendir:
      return sys_user_opendir((char *)a1);
    case SYS_user_readdir:
      return sys_user_readdir(a1, (struct dir *)a2);
    case SYS_user_mkdir:
      return sys_user_mkdir((char *)a1);
    case SYS_user_closedir:
      return sys_user_closedir(a1);
    // added @lab4_3
    case SYS_user_link:
      return sys_user_link((char *)a1, (char *)a2);
    case SYS_user_unlink:
      return sys_user_unlink((char *)a1);
    case SYS_user_rcwd:
      return sys_user_rcwd((char* )a1);
    case SYS_user_ccwd:
      return sys_user_ccwd((const char*)a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
