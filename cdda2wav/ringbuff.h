/* @(#)ringbuff.h	1.2 99/12/19 Copyright 1998,1999 Heiko Eissfeldt */
/* This file contains data structures that reside in the shared memory
 * segment.
 */


/* the linux semctl prototype is broken as is the definition
   of union semun in sys/sem.h. */

#ifdef HAVE_UNION_SEMUN
#	define	my_semun	semun
#else
union my_semun {
  int val;
  struct semid_ds *pid;
  unsigned short *array;
};
#endif

typedef struct {
  int offset;
  UINT4 data[CD_FRAMESAMPLES];
} myringbuff;

struct ringbuffheader {
  myringbuff *p1;
  myringbuff *p2;
  volatile unsigned long total_read;
  volatile unsigned long total_written;
  volatile int child_waitstate;
  volatile int parent_waitstate;
  volatile int input_littleendian;
  int offset;
  UINT4 data[CD_FRAMESAMPLES];
};

extern myringbuff **fill_buffer;
extern myringbuff **last_buffer;
extern volatile unsigned long *total_segments_read;
extern volatile unsigned long *total_segments_written;
extern volatile int *child_waits;
extern volatile int *parent_waits;
extern volatile int *in_lendian;

#define palign(x, a)    (((char *)(x)) + ((a) - 1 - (((unsigned)((x)-1))%(a))))
#define multpage(x, a)    ((((x) + (a) - 1) / (a)) * (a))

#define HEADER_SIZE multpage(offset_of(struct ringbuffheader, data), global.pagesize)
#define SEGMENT_SIZE (global.nsectors*CD_FRAMESIZE_RAW)
#define ENTRY_SIZE_PAGE_AL multpage(SEGMENT_SIZE + offset_of(myringbuff, data), global.pagesize)

#define RB_BASE ((myringbuff *)(((unsigned char *)fill_buffer) + HEADER_SIZE - offset_of(myringbuff, data)))

#define INC(a) (myringbuff *)(((char *)RB_BASE) + (((((char *) (a))-((char *)RB_BASE))/ENTRY_SIZE_PAGE_AL + 1) % total_buffers)*ENTRY_SIZE_PAGE_AL)


void set_total_buffers __PR((unsigned int num_buffers, int mysem_id));
const myringbuff *get_previous_read_buffer __PR((void));
const myringbuff *get_fill_buffer __PR((void));
myringbuff *get_next_buffer __PR((void));
myringbuff *get_oldest_buffer __PR((void));
void define_buffer __PR((void));
void drop_buffer __PR((void));
void drop_all_buffers __PR((void));
