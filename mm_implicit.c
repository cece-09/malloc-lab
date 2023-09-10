#include "mm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "week06-malloc",
    /* First member's full name */
    "SJLEE",
    /* First member's email address */
    "sojeong.lee017@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// macros
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)  // 4KB
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))  // size의 끝 비트를 0, 1로
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

// heap_listp
static void *heap_listp;
static void *last_alloc;  // 마지막으로 할당된 bp

// functions
static void *extend_heap(size_t);
static void *coalesce(void *);
static void coalesce_all();
static void place(void *, size_t);
static void *find_fit(size_t);

static void *first_fit(size_t);
static void *next_fit(size_t);

// * Initialization
int mm_init(void) {
  heap_listp = mem_sbrk(4 * WSIZE);  // 4*4byte
  if (heap_listp == (void *)-1) return -1;

  // prologue, epilogue 블록 초기화
  PUT(heap_listp, 0);
  PUT(heap_listp + (WSIZE * 1), PACK(DSIZE, 1));
  PUT(heap_listp + (WSIZE * 2), PACK(DSIZE, 1));
  PUT(heap_listp + (WSIZE * 3), PACK(0, 1));

  heap_listp += DSIZE;
  last_alloc = heap_listp;

  if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
    return -1;  // heap을 extend
  }
  return 0;
}

// * Extend Heap
static void *extend_heap(size_t words) {
  char *bp;
  size_t size;
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if ((long)(bp = mem_sbrk(size)) == -1) {
    return NULL;
  }

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // new epilogue

  return coalesce(bp);
}

// * Free
void mm_free(void *bp) {
  size_t size = GET_SIZE(HDRP(bp));

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  // coalesce(bp);
}

// * Coalesce
static void *coalesce(void *bp) {  // 해제된 가용 블록 연결
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) {
    // 인접한 가용 블록이 없다
    return bp;

  } else if (prev_alloc && !next_alloc) {
    // 뒤의 블록만 가용
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));  // 여기서 이미 bp 헤더의 size를 수정함
    PUT(FTRP(bp), PACK(size, 0));  // 기존의 next 블록 푸터(였던 것)이 수정됨

  } else if (!prev_alloc && next_alloc) {
    // 앞의 블록만 가용
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);

  } else {
    // 앞 뒤 다 가용
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  // 만약 bp가 last_alloc이면 합친 블록의 bp로
  last_alloc = bp;
  return bp;
}

// * Allocation
void *mm_malloc(size_t size) {
  char *bp;
  size_t asize;
  size_t extend_size;

  // size가 0이면 return NULL
  if (size == 0) {
    return NULL;
  }

  // 8byte 단위로 맞춤
  asize = ALIGN(size + SIZE_T_SIZE);

  // 가용블록 찾기
  bp = find_fit(asize);
  if (bp) {
    place(bp, asize);
    return bp;
  }

  // no fit이면 가능한 모든 가용블록끼리 연결: 66점
  coalesce_all();
  bp = find_fit(asize);
  if (bp) {
    place(bp, asize);
    return bp;
  }

  // 그래도 no fit이면 heap 할당
  extend_size = MAX(asize, CHUNKSIZE);
  bp = extend_heap(extend_size / WSIZE);
  if (bp == NULL)
    return NULL;
  else {
    place(bp, asize);
    return bp;
  }
}

static void coalesce_all() {
  void *bp = heap_listp;

  while (GET_SIZE(HDRP(bp)) > 0) {
    // 가용블록인 경우, 연결함
    if (!GET_ALLOC(HDRP(bp))) {
      bp = coalesce(bp);
    }
    bp = NEXT_BLKP(bp);
  }
}

// * Find Fit
static void *find_fit(size_t asize) { return next_fit(asize); }

// * First Fit으로 찾음
static void *first_fit(size_t asize) {
  void *bp;
  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
      return bp;
    }
  }
  return NULL;
}

// * Next Fit으로 찾음
static void *next_fit(size_t asize) {
  void *bp = NEXT_BLKP(last_alloc);  // 마지막 할당 블록의 다음부터
  void *start = bp;                  // 현재 시작 지점을 체크

  while (1) {
    if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
      // 발견한 경우 리턴
      return bp;
    }

    // 끝까지 가면 처음으로 돌아가서 last_alloc까지 돌아감
    if (GET_SIZE(HDRP(bp)) == 0) {
      bp = heap_listp;
    } else {
      bp = NEXT_BLKP(bp);
    }

    if (bp == start) {
      break;
    }
  }

  return NULL;  // no fit
}

// * Placement
static void place(void *bp, size_t asize) {
  size_t csize = GET_SIZE(HDRP(bp));  // 이 블록의 크기

  last_alloc = bp;  // 마지막 할당 기록

  // 블록의 크기는 최소 16바이트. 왜? 헤더 푸터가 8바이트.
  if (csize - asize >= (DSIZE * 2)) {
    // 분할이 필요한 경우
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK((csize - asize), 0));
    PUT(FTRP(bp), PACK((csize - asize), 0));
  } else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}

// * Reallocation
void *mm_realloc(void *bp, size_t size) {
  void *old_bp = bp;
  void *new_bp;
  size_t copy_size;

  // ptr이 null이면, mm_malloc(size)와 같음
  if (bp == NULL) {
    return mm_malloc(size);
  }

  // size가 0이면 mm_free(ptr)과 같음
  if (size == 0) {
    mm_free(bp);
    return NULL;
  }

  // 아니면, size만큼 새로운 블록을 할당
  new_bp = mm_malloc(size);
  if (new_bp == NULL) {
    return NULL;
  }

  copy_size = GET_SIZE(HDRP(old_bp)) - DSIZE;  // old_bp의 payload 사이즈
  if (size < copy_size) {
    copy_size = size;  // 새로 할당받는 공간이 더 적으면, 그만큼만 복사
  }
  memcpy(new_bp, old_bp, copy_size);
  mm_free(old_bp);
  return new_bp;
}
