#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>


#ifdef FS
#include <fs.h>

static struct fsystem fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0
#define BM_BLK 1
#define RT_BLK 2

#define NUM_FD 16
struct filetable oft[NUM_FD]; // open file table
int next_open_fd = 0;


#define INODES_PER_BLOCK (fsd.blocksz / sizeof(struct inode))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock);

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEBLOCKS - 2) {
    printf("No indirect block support\n");
    return SYSERR;
  }

  diskblock = oft[fd].in.blocks[fileblock]; //get the logical block address

  return diskblock;
}

/* read in an inode and fill in the pointer */
int fs_get_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;
  int inode_off;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_get_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(struct inode);

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  printf("inn*sizeof(struct inode): %d\n", inode_off);
  */

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(in, &block_cache[inode_off], sizeof(struct inode));

  return OK;

}

/* write inode indicated by pointer to device */
int fs_put_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_put_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  */

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(struct inode))], in, sizeof(struct inode));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}
     
/* create file system on device; write file system block and block bitmask to
 * device */
int fs_mkfs(int dev, int num_inodes) {
  int i;
  
  if (dev == 0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  }
  else {
    printf("Unsupported device\n");
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  }
  else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) {i++;}
  fsd.freemaskbytes = i / 8; 
  
  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *)SYSERR) {
    printf("fs_mkfs memget failed.\n");
    return SYSERR;
  }
  
  /* zero the free mask */
  for(i=0;i<fsd.freemaskbytes;i++) {
    fsd.freemask[i] = '\0';
  }
  
  fsd.inodes_used = 0;
  
  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(struct fsystem));
  
  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  return 1;
}

/* print information related to inodes*/
void fs_print_fsd(void) {

  printf("fsd.ninodes: %d\n", fsd.ninodes);
  printf("sizeof(struct inode): %d\n", sizeof(struct inode));
  printf("INODES_PER_BLOCK: %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS: %d\n", NUM_INODE_BLOCKS);
}

/* specify the block number to be set in the mask */
int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

/* specify the block number to be read in the mask */
int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
  return OK;

}

/* specify the block number to be unset in the mask */
int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/* This is maybe a little overcomplicated since the lowest-numbered
   block is indicated in the high-order bit.  Shift the byte by j
   positions to make the match in bit7 (the 8th bit) and then shift
   that value 7 times to the low-order bit to print.  Yes, it could be
   the other way...  */
void fs_printfreemask(void) { // print block bitmask
  int i,j;

  for (i=0; i < fsd.freemaskbytes; i++) {
    for (j=0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}

int fs_open(char *filename, int flags) {
  // oft[NUM_FD];
  for(int i = 0; i<next_open_fd; i++){
    if(strcmp(oft[i].de->name,filename) == 0){

      if(oft[i].state == FSTATE_OPEN){
        printf("File already open\n");
        return SYSERR;
      }

      if (flags == O_RDONLY || flags == O_WRONLY || flags == O_RDWR){
        oft[i].state = FSTATE_OPEN;
        oft[i].flag = flags;
        oft[i].fileptr = 0;

        return i;
      }

      else{
        printf("Invalid mode. Use O_RDONLY / O_WRONLY / O_RDWR\n");
        return SYSERR;
      }
    }
  }
  printf("File does not exist in File Table\n");
  return SYSERR;
}

int fs_close(int fd) {
  if (fd>=0 && fd< next_open_fd){
    if(oft[fd].state == FSTATE_CLOSED){
      printf("File already closed \n");
      return SYSERR;
    }
    oft[fd].state = FSTATE_CLOSED;
    return OK;
  }
    printf("Invalid descriptor \n");
    return SYSERR;
}

int fs_create(char *filename, int mode) {   
    struct inode in;
    // fsd.ninodes;
    if(fsd.inodes_used == fsd.ninodes){
      kprintf("No more inodes available\n");
      return SYSERR;
    }

    if(next_open_fd == NUM_FD){
      kprintf("Out of space in file table\n");
      return SYSERR;
    }

    for(int i = 0; i<next_open_fd; i++){
      if(strcmp(filename,oft[i].de->name) == 0){
        kprintf("Filename already exists\n");
        return SYSERR;
      }
    }

    if(fs_get_inode_by_num(0, ++fsd.inodes_used, &in) == SYSERR){
      return SYSERR;
    }
    in.id = fsd.inodes_used;
    in.type = INODE_TYPE_FILE;
    in.nlink = 1;
    in.size = 0;
    if(fs_put_inode_by_num(0,in.id, &in) == SYSERR){
      return SYSERR;
    }

    memcpy(fsd.root_dir.entry[fsd.root_dir.numentries].name,filename,FILENAMELEN);
    // strcpy(fsd.root_dir.entry[fsd.root_dir.numentries].name, filename);
    fsd.root_dir.entry[fsd.root_dir.numentries].inode_num = fsd.inodes_used;

    int this_ptr = next_open_fd++;
    oft[this_ptr].state = FSTATE_OPEN;
    oft[this_ptr].fileptr = 0;
    oft[this_ptr].in = in;
    oft[this_ptr].de = &fsd.root_dir.entry[fsd.root_dir.numentries++];
    oft[this_ptr].flag = O_RDWR;
    return this_ptr;
}

int fs_seek(int fd, int offset) {
  
  if (fd >= 0 && fd< next_open_fd){
    if(oft[fd].state == FSTATE_CLOSED){
      printf("File is closed \n");
      return SYSERR;
    }
  }
  else{
    printf("Invalid descriptor \n");
    return SYSERR;
  }
  int no_of_blocks = (oft[fd].fileptr + offset) / fsd.blocksz;
  if((oft[fd].fileptr + offset) % fsd.blocksz != 0)
    no_of_blocks++;
  
  if(no_of_blocks > fsd.nblocks){
    printf("offset out of scope\n");
    return SYSERR;
  }
  oft[fd].fileptr += offset;
  return OK;
}

int fs_read(int fd, void *buf, int nbytes) {
  struct inode in;
  if(fs_get_inode_by_num(0,oft[fd].in.id, &in) != OK){
      return SYSERR;
    }
  if (fd >= 0 && fd< next_open_fd){
    if(oft[fd].state == FSTATE_CLOSED){
      printf("File is closed \n");
      return SYSERR;
    }
    if(!(oft[fd].flag == O_RDWR || oft[fd].flag == O_RDONLY)){
      printf("No read permissions \n");
      return SYSERR;
    }
  }
  else{
    printf("Invalid descriptor \n");
    return SYSERR;
  }
  if(in.size == 0){
    printf("File is empty \n");
    return SYSERR;
  }
  
  int blocks_to_read = nbytes / fsd.blocksz;
  if(nbytes % fsd.blocksz != 0){
    blocks_to_read++;
  }
  blocks_to_read = (blocks_to_read < in.size ? blocks_to_read:in.size);
  int first_block_to_read = oft[fd].fileptr / fsd.blocksz;
  int blocks_read = 0;
  int offset = oft[fd].fileptr % fsd.blocksz;
  int read = 0;
  while (blocks_read < blocks_to_read){
    int bytes_to_read = fsd.blocksz - offset;
    if (offset != 0){
      if(bs_bread(dev0, in.blocks[first_block_to_read],offset,block_cache, bytes_to_read) != OK)
        return SYSERR;
      memcpy(buf+read ,block_cache,bytes_to_read);
      offset = 0;
    }
    else{
      if(bs_bread(dev0, in.blocks[first_block_to_read],offset,block_cache, bytes_to_read) != OK)
        return SYSERR;
      memcpy(buf+read ,block_cache,bytes_to_read);
      blocks_read++;
    }
    read = strlen(buf);
    first_block_to_read++;
  }
  if(fs_put_inode_by_num(0,in.id, &in) != OK){
      return SYSERR;
    }
  oft[fd].fileptr += read;
  return read;
}

int fs_write(int fd, void *buf, int nbytes) {   
  // printf("Inside write\n");
  struct inode in;
  if (fd >= 0 && fd< next_open_fd){
    if(oft[fd].state == FSTATE_CLOSED){
      printf("File is closed \n");
      return SYSERR;
    }
    if(!(oft[fd].flag == O_RDWR || oft[fd].flag == O_WRONLY)){
      printf("No write permissions \n");
      return SYSERR;
    }
  }
  else{
    printf("Invalid descriptor \n");
    return SYSERR;
  }

  int bytes_left = nbytes;
  int first_block_to_write = oft[fd].fileptr / fsd.blocksz;
  
  if(fs_get_inode_by_num(0,oft[fd].in.id, &in) != OK){
      return SYSERR;
    }

  if((oft[fd].fileptr > 0) && (oft[fd].fileptr % fsd.blocksz) != 0){ 
    int offset = oft[fd].fileptr % fsd.blocksz;
    int bytes_to_write = fsd.blocksz - offset;
    

    if(bytes_to_write < bytes_left){
      if(bs_bread(dev0, in.blocks[first_block_to_write], offset,block_cache, bytes_to_write) != OK)
        return SYSERR;
      memcpy(block_cache,buf,bytes_to_write);
      if(bs_bwrite(dev0, in.blocks[first_block_to_write], offset,block_cache, bytes_to_write) != OK)
        return SYSERR;
      bytes_left -= bytes_to_write;
      // oft[fd].fileptr += bytes_to_write;
      buf = (char*) buf + bytes_to_write;
      first_block_to_write++;
    }

    else{
      if(bs_bread(dev0, in.blocks[first_block_to_write], offset,block_cache, bytes_left) != OK)
        return SYSERR;
      memcpy(block_cache,buf,bytes_left);
      if(bs_bwrite(dev0, in.blocks[first_block_to_write], offset,block_cache, bytes_left) != OK)
        return SYSERR;
      oft[fd].fileptr += nbytes;
      // printf("fptr %d",oft[fd].fileptr);
      return nbytes;
    }
  }

  // int total_blocks = bytes_left / fsd.blocksz;
  // if(bytes_left % fsd.blocksz != 0)
  //     total_blocks++;
  int j = NUM_INODE_BLOCKS + FIRST_INODE_BLOCK;
  // int i = first_block_to_write;
  int size = in.size;
  int block = 0;

  while(bytes_left > 0 && j < fsd.nblocks){
    int bytes_to_write = (fsd.blocksz < bytes_left  ? fsd.blocksz:bytes_left);
    
    if(first_block_to_write < in.size){
      block = in.blocks[first_block_to_write++];
    }

    else{
      if(fs_getmaskbit(j) == 0){
        block = j;
        in.blocks[first_block_to_write++] = block;
        size++;
      }
      else{
        j++;
        continue;
      }
    }
    if(bs_bread(dev0, j, 0, block_cache, bytes_to_write) != OK)
        return SYSERR;
      // memcpy(&block_cache[(inn*sizeof(struct inode))], in, sizeof(struct inode));
    memcpy(block_cache, buf, bytes_to_write);
    if(bs_bwrite(dev0, j, 0, block_cache, bytes_to_write) != OK)
        return SYSERR;
      
    buf = (char*) buf + bytes_to_write;
    bytes_left -= bytes_to_write;
    fs_setmaskbit(j++);
  }
  in.size = size;
  if(fs_put_inode_by_num(0,in.id, &in) != OK){
      return SYSERR;
    }
  oft[fd].fileptr += nbytes;
  // printf("fptr %d",oft[fd].fileptr);
  return nbytes;
}


int fs_link(char *src_filename, char* dst_filename) {
  bool src_file = FALSE;
  // int src_file_index;
  struct inode in;
  for(int i = 0; i<next_open_fd; i++){

    if(strcmp(oft[i].de->name,src_filename) == 0){
      src_file  = TRUE;
      // src_file_index = i;
      if(fs_get_inode_by_num(0,oft[i].in.id, &in) != OK)
        return SYSERR;

    else if(strcmp(oft[i].de->name,dst_filename) == 0){
      printf("Destination file already exists\n");
      return SYSERR;
    }
    
  }
  if(!src_file){
    printf("Source file does not exist in File Table\n");
    return SYSERR;
  }
  int this_ptr = next_open_fd++;
  in.nlink++;
  oft[this_ptr].in = in;
  oft[this_ptr].de->inode_num = in.id;
  memcpy(oft[this_ptr].de->name,dst_filename,FILENAMELEN);
  if(fs_put_inode_by_num(0,in.id, &in) != OK)
        return SYSERR;

  return OK;
}

int fs_unlink(char *filename) {
  int i;
  if(next_open_fd > 0) { 
  for(i = 0; i<next_open_fd; i++){
      if(strcmp(oft[i].de->name,filename) == 0){
        // file found 
        int inode_number = oft[i].de->inode_num;
        struct inode *in = getmem(sizeof(struct inode));
        fs_get_inode_by_num(0,inode_number,&oft[i].in);
        if(oft[i].in.nlink==1){
              int j = oft[i].fileptr/MDEV_BLOCK_SIZE;
              int nbytes = 1200;
              int block = 0;
              while(nbytes > 0){
                block = oft[i].in.blocks[j];
                fs_clearmaskbit(block);
                nbytes -= MDEV_BLOCK_SIZE;
                j++;
              }
              fsd.inodes_used--;
        }else{
           in->nlink--;
        }
        next_open_fd--;
        fsd.root_dir.numentries--;
        return OK;
      }
  }
  kprintf("File does not exist");
  return SYSERR;
  }
  kprintf("No files");
  return SYSERR;
}
#endif /* FS */