/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "red_panda",
    /* First member's full name */
    "David McDonnel",
    /* First member's WUSTL key */
    "dmcdonnel",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's WUSTL key (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4 // size of word
#define DSIZE 8 // size of double word
#define CHUNKSIZE (1<<12) //Initial size of heap
#define OVERHEAD 8 //
#define BLKSIZE 24 //minimum block size

/* Max of two values */
#define MAX(x,y) ((x)>(y)?(x):(y))

/*pack a size and allocated bit into a word */
#define PACK(size,alloc) ((size)|(alloc))

/* read and write a word at address p*/
#define GET(p) (*(int *)(p))
#define PUT(p,val) (*(int *)(p)=(val))

/*read the size and allocated fields from address p */
#define SIZE(p) (GET(p) & ~0x7)
#define GET_HSIZE(p) (GET(HDRP(p))&~0x7)
#define GET_HALLOC(p) (GET(HDRP(p))&0x1)
#define GET_FSIZE(p) (GET(FTRP(p))&~0x7)
#define GET_FALLOC(p) (GET(FTRP(p))&0x1)

/*header and footer address in relation to bp*/
#define HDRP(bp) ((void*)(bp)-WSIZE)
#define FTRP(bp) ((void*)(bp)+GET_HSIZE(bp)-DSIZE)

/* given block pointer bp, computer address of its header and footer*/
#define NEXT_BLK(bp) ((void*)(bp)+GET_HSIZE(bp))
#define PREV_BLK(bp) ((void*)(bp) - SIZE((void*)(bp)-DSIZE))
#define NEXT_PTR(bp) (*(void**)(bp+WSIZE))
#define PREV_PTR(bp) (*(void**)(bp))

/* setters for header, footer, previous, and next of a block*/
#define SET_HDRP(bp,val) (PUT(HDRP(bp),(int)val))
#define SET_FTRP(bp,val) (PUT(FTRP(bp),(int)val))
#define SET_NEXT(bp,op) (NEXT_PTR(bp) = op)
#define SET_PREV(bp,op) (PREV_PTR(bp)=op)

/* pointer to start of heap and free list respectively*/
static char *heap_listp=0;
static char *listp=0;

/*helper function declarations*/
static void *extend_heap(size_t words);
static void place(void* bp,size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void* bp);
static void printBlock(void* bp);
static void checkBlock(void* bp);
static void mm_insert(void* bp);
static void mm_remove(void* bp);
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	if ((heap_listp=mem_sbrk(2*BLKSIZE))==NULL)
		return -1;
	PUT(heap_listp,0); //padding
	PUT(heap_listp+WSIZE,PACK(BLKSIZE,1));
	PUT(heap_listp+(2*WSIZE),0);
	PUT(heap_listp+(3*WSIZE),0);
	PUT(heap_listp+BLKSIZE,PACK(BLKSIZE,1));
	PUT(heap_listp+BLKSIZE+WSIZE,PACK(0,1));
	listp=heap_listp+DSIZE;

	//extend emtpy heap with BLKSIZE free block
	if(extend_heap(BLKSIZE)==NULL){
		return -1;
	}
	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	/*
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
    */
	size_t asize; //adjusted block size
	size_t esize; //extend size if no free block large enough
	char *bp;

	if(size<=0)
		return NULL;

	asize=MAX(ALIGN(size)+ALIGNMENT,BLKSIZE);

	//search free list
	if (bp=find_fit(asize)){
		place(bp,asize);
		return bp;
	}

	//extend since unable to allocate
	esize=MAX(asize,BLKSIZE);

	//if extend fails
	if ((bp = extend_heap(esize))==NULL)
		return NULL;
	place(bp,asize);
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	if(!ptr) return;
	size_t size = GET_HSIZE(ptr);

	//free block and coalesce
	SET_HDRP(ptr,PACK(size,0));
	SET_FTRP(ptr,PACK(size,0));
	coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	if(size<=0){
		mm_free(ptr);
		return NULL;
	}
	else if (ptr==NULL){
		ptr = mm_malloc(size);
		return ptr;
	}

	if(size>0){
		size_t csize = GET_HSIZE(ptr);
		size_t nsize = ALIGN(size+OVERHEAD);

		//new size < current size then return ptr
		if(nsize<=csize)
			return ptr;
		else{
			size_t next_alloc = GET_HALLOC(NEXT_BLK(ptr));
			size_t curr;
			size_t asize;

			//next block free and size of blocks >= new size
			if (!next_alloc&&((curr = csize+GET_HSIZE(NEXT_BLK(ptr))))>=nsize){
				mm_remove(NEXT_BLK(ptr));
				SET_HDRP(ptr,PACK(curr,1));
				SET_FTRP(ptr,PACK(curr,1));
				return ptr;
			}//next block is free and last block before epilogue
			else if(!next_alloc&&((GET_HSIZE(NEXT_BLK(NEXT_BLK(ptr))))==0)){
				curr = nsize-csize+GET_HSIZE(NEXT_BLK(ptr));
				void* tmp = extend_heap(curr);
				asize=csize+GET_HSIZE(tmp);
				SET_HDRP(ptr,PACK(asize,1));
				SET_FTRP(ptr,PACK(asize,1));
				return ptr;
			}//ptr is last block before epilogue
			else if(GET_HSIZE(NEXT_BLK(ptr))==0){
				curr = nsize-csize;
				void* tmp = extend_heap(curr);
				asize=csize+GET_HSIZE(tmp);
				SET_HDRP(ptr,PACK(asize,1));
				SET_FTRP(ptr,PACK(asize,1));
				return ptr;
			}//extend heap since no free space
			else{
				void* np = mm_malloc(nsize);
				place(np,nsize);
				memcpy(np,ptr,nsize);
				mm_free(ptr);
				return np;
			}
		}
	}else
		return NULL;
}

/**
 * check heap for consistency,
 * @param - int verbose
 */
void mm_checkheap(int verbose){
	void* bp = heap_listp;

	if(verbose){
		printf("Heap (%p):\n", heap_listp);
	}
	if(((GET_HSIZE(heap_listp))!=BLKSIZE)||!GET_HALLOC(heap_listp)){
		printf("Bad prologue header\n");
	}
	checkBlock(heap_listp);

	for (bp = listp; GET_HALLOC(bp)==0;bp=NEXT_PTR(bp)){
		if(verbose){
			printBlock(bp);
		}
		checkBlock(bp);
	}

	if ((GET_HSIZE(bp)!=0)||!(GET_HALLOC(bp))){
		printf("Bad epilogue header\n");
	}
}

/**
 * extend_heap - Extend heap w/ free block
 * Allocates a new free block of size after the last block in the heap
 * If the last block is free merge with new free block
 * Creates an epilogue block after newly allocated block
 * @param - words - extend heap by size 4*words
 * @return - bp - pointer to the new free block
 */
static void* extend_heap(size_t words){
	char* bp;
	size_t size;

	//Allocate an even number of words to maintain alignment
	size = (words%2)?(words+1)*WSIZE:words*WSIZE;

	if(size<BLKSIZE)
		size = BLKSIZE;
	if ((int)(bp=mem_sbrk(size))==-1)
		return NULL;
	SET_HDRP(bp,PACK(size,0)); //block header
	SET_FTRP(bp,PACK(size,0)); //block footer
	SET_HDRP(NEXT_BLK(bp),PACK(0,1)); //epilogue header

	return coalesce(bp);
}

/**
 * place - put block of asize bytes at begininning of free block bp
 * split if remainder at least minimum block size
 * @param - bp - pointer to free block
 * @param - asize - size of free block
 */
static void place(void* bp,size_t asize){
	size_t csize = GET_HSIZE(bp);

	if ((csize - asize)>=BLKSIZE){
		SET_HDRP(bp,PACK(asize,1));
		SET_FTRP(bp,PACK(asize,1));
		mm_remove(bp);
		bp=NEXT_BLK(bp);
		SET_HDRP(bp,PACK(csize-asize,0));
		SET_FTRP(bp,PACK(csize-asize,0));
		coalesce(bp);
	}
	else{
		SET_HDRP(bp,PACK(csize,1));
		SET_FTRP(bp,PACK(csize,1));
		mm_remove(bp);
	}
}

/**
 * find_fit - Search for free block w/ size >= asize
 * remove block from list
 * if block found return it, else return NULL
 * @param - asize - size block from free list
 * @return bp - pointer to best fit block
 */
static void* find_fit(size_t asize){
	void* bp;

	for(bp=listp; GET_HALLOC(bp)==0;bp=NEXT_PTR(bp)){
		if(asize<=(size_t)GET_HSIZE(bp))
			return bp;
	}
	return NULL;
}

/**
 * coalesce - using boundary tag, remove adjacent blocks
 * from free list if one or both are free,
 * then merge adjacent blocks into one free block
 * then insert it into free list
 * @return - bp - pointer to merged free block
 */
static void* coalesce(void* bp){
	size_t prev_alloc=GET_FALLOC(PREV_BLK(bp))||PREV_BLK(bp)==bp;
	size_t next_alloc = GET_HALLOC(NEXT_BLK(bp));
	size_t size = GET_HSIZE(bp);

	//next block free
	if(prev_alloc&&!next_alloc){
		size+=GET_HSIZE(NEXT_BLK(bp));
		mm_remove(NEXT_BLK(bp));
		SET_HDRP(bp,PACK(size,0));
		SET_FTRP(bp,PACK(size,0));
	}//prev block free
	else if (!prev_alloc&&next_alloc){
		size+=GET_HSIZE(PREV_BLK(bp));
		bp=PREV_BLK(bp);
		mm_remove(bp);
		SET_HDRP(bp,PACK(size,0));
		SET_FTRP(bp,PACK(size,0));
	}//both adjacent blocks free
	else if (!prev_alloc&&!next_alloc){
		size+=GET_HSIZE(PREV_BLK(bp))+GET_HSIZE(NEXT_BLK(bp));
		mm_remove(PREV_BLK(bp));
		mm_remove(NEXT_BLK(bp));
		bp = PREV_BLK(bp);
		SET_HDRP(bp,PACK(size,0));
		SET_FTRP(bp,PACK(size,0));
	}
	mm_insert(bp);
	return bp;
}

/**
 * printBlock - print block 
 * @param - bp - pointer to block
 */
static void printBlock(void* bp){
	size_t hsize = GET_HSIZE(bp);
	size_t halloc = GET_HALLOC(bp);
	size_t fsize = GET_HSIZE(bp);
	size_t falloc = GET_HALLOC(bp);

	if(hsize==0){
		printf("%p: EOL\n", bp);
		return;
	}
	if (halloc){
		printf("%p: header:[%d:%c] footer:[%d:%c]\n",bp,
		hsize, (halloc ? 'a' : 'f'),
		fsize, (falloc ? 'a' : 'f'));
	}
	else{
		printf("%p:header:[%d:%c] prev:%p next:%p footer: [%d:%c]\n",
		bp, hsize, (halloc ? 'a' : 'f'),
		PREV_PTR(bp), NEXT_PTR(bp),
		fsize, (falloc ? 'a' : 'f'));
	}
}

/**
 * checkBlock - check block alignment
 * @param - bp - pointer to block
 */
static void checkBlock(void* bp){
	if ((size_t)bp %8)
		printf("Error: %p is not doubleword aligned\n",bp);
	if (GET(HDRP(bp))!=GET(FTRP(bp)))
		printf("Error: header and footer do not match\n");
}

/**
 * mm_insert - add block to free list
 * @param - bp - pointer to block
 */
static void mm_insert(void* bp){
	SET_NEXT(bp,listp);
	SET_PREV(listp,bp);
	SET_PREV(bp,NULL);
	listp=bp;
}

/**
 * mm_remove - remove block from free list
 * @param - bp - block pointer
 */
static void mm_remove(void* bp){
	if(PREV_PTR(bp)){
		SET_NEXT(PREV_PTR(bp), NEXT_PTR(bp));
	}
	else{
		listp=NEXT_PTR(bp);
	}
	SET_PREV(NEXT_PTR(bp),PREV_PTR(bp));
}










