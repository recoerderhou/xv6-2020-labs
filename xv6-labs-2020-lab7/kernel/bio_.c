// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

struct bucket {
	struct spinlock lock;
	struct buf head;
} bucketlock[NBUCKET];

uint hash(uint block_num){
	return block_num % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
  }
  
  b = bcache.buf;
  for(int i = 0; i < NBUCKET; ++ i){
  	initlock(&bucketlock[i].lock, "bucket_lock");
  	for(int j = 0; j < NBUF / NBUCKET; ++ j){
  		b->blockno = i;
  		b->next = bucketlock[i].head.next;
  		bucketlock[i].head.next = b;
  		++ b;
  	}
  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // acquire(&bcache.lock);
  int index = hash(blockno);
  struct bucket* bucket = bucketlock + index;
  // printf("bget: bucket\n");
  acquire(&bucket->lock);

  // Is the block already cached?
  for(b = bucket->head.next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket->lock);
      // printf("bget: acquiresleep1\n");
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  int min_time = 0x8fffffff;
  struct buf* replace_buf = 0;
  for(b = bucket->head.next; b != 0; b = b->next){
    if(b->refcnt == 0 && b->timestamp < min_time) {
      replace_buf = b;
      min_time = b->timestamp;
    }
  }
  if(replace_buf){
  	goto find;
  }
  
  acquire(&bcache.lock);
  refind:
  for(b = bcache.buf; b < bcache.buf + NBUF; ++b){
  	if(b->refcnt == 0 && b->timestamp < min_time) {
      replace_buf = b;
      min_time = b->timestamp;
    }
  }
  if(replace_buf){
  	int replace_index = hash(blockno);
  	printf("bget: bucketlock %d, orignally in %d\n", replace_index, index);
  	if(replace_index != index)
  		acquire(&bucketlock[replace_index].lock);
  	if(replace_buf->refcnt != 0){
  		release(&bucketlock[replace_index].lock);
  		goto refind;
  	}
  	struct buf *pre = &bucketlock[replace_index].head;
    struct buf *p = bucketlock[replace_index].head.next;
    while (p != replace_buf) {
      pre = pre->next;
      p = p->next;
    }
    pre->next = p->next;
    if(replace_index != index)
    	release(&bucketlock[replace_index].lock);
    replace_buf->next = bucketlock[replace_index].head.next;
    bucketlock[index].head.next = replace_buf;
    release(&bcache.lock);
    goto find;
  }
  else{
  	panic("bget: no buffers");
  }
  find:
  replace_buf->dev = dev;
  replace_buf->blockno = blockno;
  replace_buf->valid = 0;
  replace_buf->refcnt = 1;
  release(&bucket->lock);
  // printf("bget: acquiresleep2\n");
  acquiresleep(&replace_buf->lock);
  return replace_buf;
  
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  
  releasesleep(&b->lock);
  
  int index = hash(b->blockno);
  acquire(&bucketlock[index].lock);

  // acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->timestamp = ticks;
  }
  release(&bucketlock[index].lock);
  // release(&bcache.lock);
}

void
bpin(struct buf *b) {
	int index = hash(b->blockno);
	acquire(&bucketlock[index].lock);
  b->refcnt++;
  release(&bucketlock[index].lock);
}

void
bunpin(struct buf *b) {
  int index = hash(b->blockno);
	acquire(&bucketlock[index].lock);
  b->refcnt--;
  release(&bucketlock[index].lock);
}


