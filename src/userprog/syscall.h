#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/kernel/list.h"


typedef int pid_t;

struct file_node {
  int fd;
  struct file *fp;
  struct thread *thread;
  struct list_elem elem;
};


// FILE DESCRIPTOR FUNCTION

int make_fd( void );
struct file_node *find_file_node_by_fd( int );
struct file *find_file_by_fd( int );
void close_file_by_thread( struct thread * );


// SYSTEM CALL FUNCTION

void syscall_init (void);

void sys_halt( void );
int sys_exit( int );
pid_t sys_exec( const char * );
int sys_wait( pid_t );
bool sys_create( const char *, unsigned );
bool sys_remove( const char * );
int sys_open( const char * );
int sys_filesize( int );
int sys_read( int, void *, unsigned );
int sys_write( int, const void *, unsigned );
void sys_seek( int, unsigned );
unsigned sys_tell( int );
void sys_close( int );

#endif /* userprog/syscall.h  */
