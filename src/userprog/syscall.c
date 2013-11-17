#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/init.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "userprog/process.h"

/* This is a skeleton system call handler */

static void syscall_handler (struct intr_frame *);


static struct list file_list;
static struct lock file_lock;
static int fd;

// FILE DESCRIPTOR FUNCTION

int make_fd() {
  int ret = fd;
  fd ++;

  return ret;
}

struct file_node *find_file_node_by_fd( int fd ){
  struct list_elem *e;

  for( e = list_begin( &file_list ); e != list_end( &file_list ); e = list_next( e ) ) {
    struct file_node *fnode = list_entry( e, struct file_node, elem );
    if( fnode->fd == fd ) {
      return fnode;
    }
  }
  
  return NULL;
}

struct file *find_file_by_fd( int fd ){
  struct file_node *e;

  e = find_file_node_by_fd( fd );
  if( e == NULL ) {
    return NULL;
  }
  return e->fp;
}

void close_file_by_thread( struct thread * thread ) {
  struct list_elem *e;
  int before_fd = -1;

  for( e = list_begin( &file_list ); e != list_end( &file_list ); e = list_next( e ) ) {
    struct file_node *fnode = list_entry( e, struct file_node, elem );
    if( fnode->thread == thread ) {
      if( before_fd != -1 ) {
        sys_close( before_fd );
      }
      before_fd = fnode->fd;
    }
  }
  if( before_fd != -1 ) {
    sys_close( before_fd );
  }
}


// SYSTEM CALL FUNCTION

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
 
  list_init( &file_list );
  lock_init( &file_lock );
  fd = 2;
}

static void
syscall_handler (struct intr_frame *f) 
{
  if( !is_user_vaddr( (void *)(f->esp) ) ) {
    sys_exit( -1 );
  }
  int syscall_number = *(int *)(f->esp);
  int arg1, arg2, arg3;

  arg1 = arg2 = arg3 = 0;

  if( syscall_number == SYS_EXIT || 
      syscall_number == SYS_EXEC ||
      syscall_number == SYS_WAIT ||
      syscall_number == SYS_CREATE || 
      syscall_number == SYS_REMOVE ||
      syscall_number == SYS_OPEN || 
      syscall_number == SYS_FILESIZE ||
      syscall_number == SYS_READ || 
      syscall_number == SYS_WRITE ||
      syscall_number == SYS_SEEK ||
      syscall_number == SYS_TELL ||
      syscall_number == SYS_CLOSE ) {
    if( !is_user_vaddr( (void *)(f->esp + 4) ) ) {
      sys_exit( -1 );
    }
    arg1 = *(int *)(f->esp+4);
  }
  if( syscall_number == SYS_CREATE ||
      syscall_number == SYS_READ || 
      syscall_number == SYS_WRITE || 
      syscall_number == SYS_SEEK ) {
    if( !is_user_vaddr( (void *)(f->esp + 8) ) ) {
      sys_exit( -1 );
    }
    arg2 = *(int *)(f->esp+8);
  }
  if( syscall_number == SYS_READ ||
      syscall_number == SYS_WRITE ) {
    if( !is_user_vaddr( (void *)(f->esp + 12) ) ) {
      sys_exit( -1 );
    }
    arg3 = *(int *)(f->esp+12);
  }

  switch( syscall_number ) {
    case SYS_HALT: 
      sys_halt();
      break;
    case SYS_EXIT:
      f->eax = sys_exit( arg1 );
      break;
    case SYS_EXEC:
      f->eax = sys_exec( (const char *) arg1 );
      break;
    case SYS_WAIT: 
      f->eax = sys_wait( (pid_t) arg1 );
      break;
    case SYS_CREATE:
      f->eax = sys_create( (const char *) arg1, (unsigned) arg2 );
      break;
    case SYS_REMOVE:
      f->eax = sys_remove( (const char*) arg1 );
      break;
    case SYS_OPEN:
      f->eax = sys_open( (const char*) arg1 );
      break;
    case SYS_FILESIZE:
      f->eax = sys_filesize( arg1 );
      break;
    case SYS_READ:
      f->eax = sys_read( arg1, (void *) arg2, (unsigned) arg3 );
      break;
    case SYS_WRITE:
      f->eax = sys_write( arg1, (const void *) arg2, (unsigned) arg3 );
      return;
    case SYS_SEEK:
      sys_seek( arg1, (unsigned) arg2 );
      break;
    case SYS_TELL:
      f->eax = sys_tell( arg1 );
      break;
    case SYS_CLOSE:
      sys_close( arg1 );
      break;
    default:
      printf( "System call %d (Not Implemented)\n", syscall_number);
      thread_exit();
      break;
  }
}

void sys_halt() {
  power_off();
}

int sys_exit( int status ) {
  struct thread *thread = thread_current();

  close_file_by_thread( thread );

  thread->ret_status = status;
  thread_exit();

  return -1;
}

pid_t sys_exec( const char *file ) {
  if( file == NULL || ! is_user_vaddr( file ) ) {
    return -1;
  }
  pid_t pid;

  pid = process_execute( file );

  return pid;
}

int sys_wait( pid_t pid ) {
  return process_wait( pid );
}

bool sys_create( const char *file, unsigned initial_size ) {
  if( file == NULL ) {
    sys_exit( -1 );
  }
  if( ! is_user_vaddr( file ) ) {
    return false;
  }
  return filesys_create( file, initial_size );
}

bool sys_remove( const char *file ) {
  if( file == NULL || ! is_user_vaddr( file ) ) {
    return false;
  }
  return filesys_remove( file );
}

int sys_open( const char *file ) {
  int fd;
  struct file *fp;
  struct file_node *fnode;

  if( file == NULL || ! is_user_vaddr( file ) ) {
    return -1;
  }

  fp = filesys_open( file );
  if( fp == NULL ) {
    return -1;
  }

  fnode = (struct file_node *) malloc( sizeof( struct file_node ) );
  if( fnode == NULL ) {
    file_close( fp );
    return -1;
  }

  fd = make_fd();

  fnode->thread = thread_current();
  fnode->fd = fd;
  fnode->fp = fp;
  list_push_back( &file_list, &fnode->elem );
  
  return fd;
}

int sys_filesize( int fd ) {
  struct file *fp = find_file_by_fd( fd );
  if( fp == NULL ) {
    return -1;
  } 
  
  return file_length( fp );
}

int sys_read( int fd, void *buffer, unsigned size ) {
  struct file *fp;
  char *input_buffer;
  int ret = -1;

  if( ! is_user_vaddr( buffer ) || ! is_user_vaddr( buffer + size ) ) {
    // buffer needs space for null('\0')
    return -1;
  }

  input_buffer = (char *) buffer;

  lock_acquire( &file_lock );
  if( fd == STDIN_FILENO ) {
    unsigned i;
    for( i = 0; i < size; i ++ ) {
      input_buffer[ i ] = (char) input_getc();
    }
    ret = size;
  } else if( fd != STDOUT_FILENO ) {
    fp = find_file_by_fd( fd );
    if( fp != NULL ) {
      ret = file_read( fp, buffer, size );
    }
  }
  lock_release( &file_lock );

  return ret;
}

int sys_write( int fd, const void *buffer, unsigned size ) {
  struct file *fp;
  char *output_buffer;
  int ret = -1;

  if( ! is_user_vaddr( buffer ) || ! is_user_vaddr( buffer + size - 1 ) ) {
    return -1;
  }

  output_buffer = (char *) buffer;

  lock_acquire( &file_lock );
  if( fd == STDOUT_FILENO ) {
    putbuf( (const char *) buffer, size );
    ret = size;
  } else {
    fp = find_file_by_fd( fd );
    if( fp != NULL ) {
      ret = file_write( fp, buffer, size );
    }
  }
  lock_release( &file_lock );

  return ret;
}

void sys_seek( int fd, unsigned position ) {
  struct file *fp = find_file_by_fd( fd );
  if( fp == NULL ) {
    return;
  }

  file_seek( fp, position );
}

unsigned sys_tell( int fd ) {
  struct file *fp = find_file_by_fd( fd );
  if( fp == NULL ) {
    return 0;
  }

  return file_tell( fp );
}

void sys_close( int fd ) {
  struct file_node *fnode = find_file_node_by_fd( fd );
  if( fnode == NULL ) {
    return;
  }
 
  file_close( fnode->fp );
  list_remove( &fnode->elem );
  free( fnode );
}
