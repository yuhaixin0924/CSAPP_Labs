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
#include <stdint.h>

#include "mm.h"
#include "memlib.h"



/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7) // 向上对齐，保证分配的空间是8的倍数

#define SIZE_T_SIZE (ALIGN(sizeof(size_t))) // header or footer size

/* A block must have room for one header and at least one aligned word. */
#define MIN_BLOCK_SIZE (2 * SIZE_T_SIZE + ALIGNMENT) // header + minimum aligned payload + footer

#define PACK(size, allocated) ((size) | (allocated)) // block size + allocated state

#define GET(p) (*(size_t *)(p)) // get header or footer

#define GET_SIZE(p) (GET(p) & ~((size_t)ALIGNMENT - 1)) // get malloc block size

#define GET_ALLOC(p) (GET(p) & 0x1) // get allocated state

#define PUT(p, value) (*(size_t *)(p) = (value)) // put value on header or footer

int mm_checkheap(void);

#ifdef DEBUG
#define CHECK_HEAP() assert(mm_checkheap())
#else
#define CHECK_HEAP() ((void)0)
#endif
/*
 * mm_init - initialize the malloc package
 */
int mm_init(void)
{
    return 0;
}
static void *find_fit(size_t needed_size)
{
    if (mem_heapsize() == 0)
    { // 当前堆还没有分配块，是空堆
        return NULL;
    }
    char *header = (char *)mem_heap_lo();   // 返回模拟堆的初始地址
    char *heap_end = (char *)mem_heap_hi(); // 返回模拟堆的结尾地址

    while (header <= heap_end)
    {
        size_t block_size = GET_SIZE(header);
        int allocated = GET_ALLOC(header);

        if (!allocated && block_size >= needed_size)
        {
            return header + SIZE_T_SIZE; // 当前header指针指向块开头，但是应返回payload
        }
        header += block_size;
    }
    return NULL;
}

/*
 * Allocate needed_size bytes from an existing free block.  If the unused
 * tail is large enough to be a useful block, leave that tail free.
 */
static void place(void *payload, size_t needed_size) // 注意这里的needed_size是指整块的大小，不只是payload的大小
{

    char *header = (char *)payload - SIZE_T_SIZE;
    size_t block_size = GET_SIZE(header);
    size_t remainder = block_size - needed_size;

    if (remainder >= MIN_BLOCK_SIZE)
    {                                                                 // 如果剩余大小可以切割就切割
        char *newfooter = (char *)header + needed_size - SIZE_T_SIZE; // 更新的footer
        PUT(header, PACK(needed_size, 1));
        PUT(newfooter, PACK(needed_size, 1));
        // 新开一块块
        char *remainder_header = header + needed_size;
        char *remainder_footer = header + block_size - SIZE_T_SIZE;
        PUT(remainder_header, PACK(remainder, 0));
        PUT(remainder_footer, PACK(remainder, 0));
    }
    else
    { // 不切割，块还是保留原来大小
        char *footer = header + block_size - SIZE_T_SIZE;
        PUT(header, PACK(block_size, 1));
        PUT(footer, PACK(block_size, 1));
    }
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) // size指的是payload大小
{
    if (size == 0)
    {
        return NULL;
    }
    size_t newsize = ALIGN(size + 2 * SIZE_T_SIZE);
    /*
     * 先尝试复用以前释放的块。
     */
    void *p = find_fit(newsize);

    if (p != NULL)
    {
        place(p, newsize);
        CHECK_HEAP();
        return p;
    }

    /*
     * 没有合适的空闲块，再扩展堆。
     */

    p = mem_sbrk((int)newsize);

    if (p == (void *)-1)
        return NULL;

    char *header = (char *)p;
    char *footer = header + newsize - SIZE_T_SIZE;
    PUT(header, PACK(newsize, 1));
    PUT(footer, PACK(newsize, 1));
    CHECK_HEAP();
    return (void *)((char *)p + SIZE_T_SIZE);
}

static void *coalesce(void *payload)
{
    char *heap_start = (char *)mem_heap_lo();
    char *heap_limit = (char *)mem_heap_hi() + 1;

    char *header = (char *)payload - SIZE_T_SIZE;
    size_t block_size = GET_SIZE(header);

    char *next_header = header + block_size;

    int has_previous = header > heap_start;
    int has_next = next_header < heap_limit;

    int previous_allocated = 1;
    int next_allocated = 1;

    char *previous_header = NULL; // 目前还不知道previous_size,所以不知道previous_header
    size_t previous_size = 0;
    size_t next_size = 0;

    if (has_previous)
    {
        char *previous_footer = header - SIZE_T_SIZE;
        previous_size = GET_SIZE(previous_footer);
        previous_allocated = GET_ALLOC(previous_footer);
        previous_header = header - previous_size;
    }
    if (has_next)
    {
        next_size = GET_SIZE(next_header);
        next_allocated = GET_ALLOC(next_header);
    }
    if (previous_allocated && next_allocated)
    {
        return payload;
    }
    else if (previous_allocated && !next_allocated)
    {
        size_t merge_size = next_size + block_size;
        char *footer = header + merge_size - SIZE_T_SIZE;
        PUT(header, PACK(merge_size, 0));
        PUT(footer, PACK(merge_size, 0));
        return payload;
    }
    else if (!previous_allocated && next_allocated)
    {
        size_t merget_size = block_size + previous_size;
        char *footer = previous_header + merget_size - SIZE_T_SIZE;
        PUT(previous_header, PACK(merget_size, 0));
        PUT(footer, PACK(merget_size, 0));
        return previous_header + SIZE_T_SIZE;
    }
    else
    {
        size_t merget_size = block_size + previous_size + next_size;
        char *footer = previous_header + merget_size - SIZE_T_SIZE;
        PUT(previous_header, PACK(merget_size, 0));
        PUT(footer, PACK(merget_size, 0));
        return previous_header + SIZE_T_SIZE;
    }
}

/*
 * Check the physical heap layout.  Return 1 when every block is valid and
 * 0 after printing the first invariant violation found.
 */
int mm_checkheap(void)
{
    if (mem_heapsize() == 0)
    {
        return 1;
    }

    char *heap_start = (char *)mem_heap_lo();
    char *heap_limit = (char *)mem_heap_hi() + 1;
    char *header = heap_start;
    int previous_was_free = 0;

    while (header < heap_limit)
    {
        size_t remaining = (size_t)(heap_limit - header);

        if (remaining < SIZE_T_SIZE)
        {
            fprintf(stderr, "heap check: incomplete header at %p\n",
                    (void *)header);
            return 0;
        }

        size_t header_value = GET(header);
        size_t block_size = GET_SIZE(header);
        int allocated = GET_ALLOC(header);

        if (block_size < MIN_BLOCK_SIZE)
        {
            fprintf(stderr, "heap check: block at %p is too small (%zu)\n",
                    (void *)header, block_size);
            return 0;
        }

        if (block_size % ALIGNMENT != 0)
        {
            fprintf(stderr, "heap check: block at %p is not aligned (%zu)\n",
                    (void *)header, block_size);
            return 0;
        }

        if (block_size > remaining)
        {
            fprintf(stderr, "heap check: block at %p crosses the heap end\n",
                    (void *)header);
            return 0;
        }

        char *payload = header + SIZE_T_SIZE;
        char *footer = header + block_size - SIZE_T_SIZE;

        if ((uintptr_t)payload % ALIGNMENT != 0)
        {
            fprintf(stderr, "heap check: payload at %p is not aligned\n",
                    (void *)payload);
            return 0;
        }

        if (GET(footer) != header_value)
        {
            fprintf(stderr,
                    "heap check: header/footer mismatch for block at %p\n",
                    (void *)header);
            return 0;
        }

        if (!allocated && previous_was_free)
        {
            fprintf(stderr,
                    "heap check: adjacent free blocks remain at %p\n",
                    (void *)header);
            return 0;
        }

        previous_was_free = !allocated;
        header += block_size;
    }

    if (header != heap_limit)
    {
        fprintf(stderr, "heap check: traversal did not end at heap limit\n");
        return 0;
    }

    return 1;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    char *header = (char *)ptr - SIZE_T_SIZE;
    size_t block_size = GET_SIZE(header); // 得到整个block size
    char *footer = header + block_size - SIZE_T_SIZE;

    PUT(header, PACK(block_size, 0)); // 更新分配状态,无需删除payload中的数据
    PUT(footer, PACK(block_size, 0));
    coalesce(ptr);
    CHECK_HEAP();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) // size指的是payload大小
{
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }

    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    char *header = (char *)ptr - SIZE_T_SIZE;
    size_t old_block_size = GET_SIZE(header);
    size_t needed_size = ALIGN(size + 2 * SIZE_T_SIZE);

    /*
     * The current block is already large enough.  Returning it unchanged is
     * correct.  Optional TODO: split a sufficiently large unused tail.
     */
    if (needed_size <= old_block_size)
    {
        CHECK_HEAP();
        return ptr;
    }

    char *heap_limit = (char *)mem_heap_hi() + 1;
    char *next_header = header + old_block_size;
    /*
     * Grow in place by consuming the free physical block on the right.
     * place() splits the combined block when the remainder is large enough.
     */
    // 右边有空闲块，合并后大小大于等于所需要调整的大小
    if (next_header < heap_limit && !GET_ALLOC(next_header))
    {
        size_t next_size = GET_SIZE(next_header);
        size_t combined_size = old_block_size + next_size;

        if (combined_size >= needed_size)
        {
            char *footer = header + combined_size - SIZE_T_SIZE;
            PUT(header, PACK(combined_size, 1));
            PUT(footer, PACK(combined_size, 1));
            place(ptr, needed_size);
            CHECK_HEAP();
            return ptr;
        }
    }

    void *newptr = mm_malloc(size);
    if (newptr == NULL)
    {
        return NULL;
    }

    size_t copy_size = old_block_size - 2 * SIZE_T_SIZE;
    if (size < copy_size)
    {
        copy_size = size;
    }

    memcpy(newptr, ptr, copy_size);
    mm_free(ptr);
    return newptr;
}
