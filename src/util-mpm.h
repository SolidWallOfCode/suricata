/* Copyright (c) 2008 Victor Julien <victor@inliniac.net> */

#ifndef __UTIL_MPM_H__
#define __UTIL_MPM_H__

#define MPM_ENDMATCH_SINGLE   0x01 /* A single match is sufficient. No depth, offset, etc settings. */
#define MPM_ENDMATCH_OFFSET   0x02 /* has offset setting */
#define MPM_ENDMATCH_DEPTH    0x04 /* has depth setting */
#define MPM_ENDMATCH_NOSEARCH 0x08 /* if this matches, no search is required (for this pattern) */

enum {
    MPM_WUMANBER,
    MPM_B2G,
    MPM_B3G,

    /* table size */
    MPM_TABLE_SIZE,
};

/* Data structures */
typedef struct MpmEndMatch_ {
    uint32_t id;
    uint16_t depth;
    uint16_t offset;
    uint8_t flags;
    struct MpmEndMatch_ *next;
    uint32_t sig_id; /* sig callback stuff -- internal id */
} MpmEndMatch;

typedef struct MpmMatch_ {
    uint16_t offset; /* offset of this match in the search buffer */
    struct MpmMatch_ *next; /* match list -- used to connect a match to a
                             * pattern id. */
    struct MpmMatch_ *qnext; /* queue list -- used to cleanup all matches after
                              * the inspection. */
    struct MpmMatchBucket_ *mb; /* pointer back to the bucket */
} MpmMatch;

typedef struct MpmMatchBucket_ {
    MpmMatch *top;
    MpmMatch *bot;
    uint32_t len;
} MpmMatchBucket;

typedef struct MpmThreadCtx_ {
    void *ctx;

    uint32_t memory_cnt;
    uint32_t memory_size;

    MpmMatchBucket *match;
    /* list of all matches */
    MpmMatch *qlist;
    /* spare list */
    MpmMatch *sparelist;

    uint32_t matches;

} MpmThreadCtx;

#define PMQ_MODE_SCAN   0
#define PMQ_MODE_SEARCH 1

/* helper structure for the detection engine. The Pattern Matcher thread
 * has this and passes a pointer to it to the pattern matcher. The actual
 * pattern matcher will fill the structure. */
typedef struct PatternMatcherQueue_ {
    /* sig callback stuff XXX consider a separate struct for this*/
    uint32_t *sig_id_array; /* array with internal sig id's that had a
                                pattern match. These will be inspected
                                futher by the detection engine. */
    uint32_t sig_id_array_cnt;
    uint8_t *sig_bitarray;
    char mode; /* 0: scan, 1: search */
    uint32_t searchable; /* counter of the number of matches that
                             require a search-followup */
} PatternMatcherQueue;

typedef struct MpmCtx_ {
    void *ctx;

    void (*InitCtx)(struct MpmCtx_ *);
    void (*InitThreadCtx)(struct MpmCtx_ *, struct MpmThreadCtx_ *, uint32_t);
    void (*DestroyCtx)(struct MpmCtx_ *);
    void (*DestroyThreadCtx)(struct MpmCtx_ *, struct MpmThreadCtx_ *);
    int  (*AddScanPattern)(struct MpmCtx_ *, uint8_t *, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t, uint8_t);
    int  (*AddScanPatternNocase)(struct MpmCtx_ *, uint8_t *, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t, uint8_t);
    int  (*AddPattern)(struct MpmCtx_ *, uint8_t *, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t);
    int  (*AddPatternNocase)(struct MpmCtx_ *, uint8_t *, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t);
    int  (*Prepare)(struct MpmCtx_ *);
    uint32_t (*Scan)(struct MpmCtx_ *, struct MpmThreadCtx_ *, PatternMatcherQueue *, uint8_t *, uint16_t);
    uint32_t (*Search)(struct MpmCtx_ *, struct MpmThreadCtx_ *, PatternMatcherQueue *, uint8_t *, uint16_t);
    void (*Cleanup)(struct MpmThreadCtx_ *);
    void (*PrintCtx)(struct MpmCtx_ *);
    void (*PrintThreadCtx)(struct MpmThreadCtx_ *);

    uint32_t memory_cnt;
    uint32_t memory_size;

    uint32_t endmatches;

    uint32_t scan_pattern_cnt;  /* scan patterns */
    uint32_t pattern_cnt;       /* unique patterns */
    uint32_t total_pattern_cnt; /* total patterns added */

    uint16_t scan_minlen;
    uint16_t scan_maxlen;
    uint16_t search_minlen;
    uint16_t search_maxlen;

    /* this is used to determine the size of the match
     * loopup table */
    uint32_t max_pattern_id;

} MpmCtx;

typedef struct MpmTableElmt_ {
    char *name;
    void (*InitCtx)(struct MpmCtx_ *);
    void (*InitThreadCtx)(struct MpmCtx_ *, struct MpmThreadCtx_ *, uint32_t);
    void (*DestroyCtx)(struct MpmCtx_ *);
    void (*DestroyThreadCtx)(struct MpmCtx_ *, struct MpmThreadCtx_ *);
    int  (*AddScanPattern)(struct MpmCtx_ *, uint8_t *, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t, uint8_t);
    int  (*AddScanPatternNocase)(struct MpmCtx_ *, uint8_t *, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t, uint8_t);
    int  (*AddPattern)(struct MpmCtx_ *, uint8_t *, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t);
    int  (*AddPatternNocase)(struct MpmCtx_ *, uint8_t *, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t);
    int  (*Prepare)(struct MpmCtx_ *);
    uint32_t (*Scan)(struct MpmCtx_ *, struct MpmThreadCtx_ *, PatternMatcherQueue *, uint8_t *, uint16_t);
    uint32_t (*Search)(struct MpmCtx_ *, struct MpmThreadCtx_ *, PatternMatcherQueue *, uint8_t *, uint16_t);
    void (*Cleanup)(struct MpmThreadCtx_ *);
    void (*PrintCtx)(struct MpmCtx_ *);
    void (*PrintThreadCtx)(struct MpmThreadCtx_ *);
    void (*RegisterUnittests)(void);
    uint8_t flags;
} MpmTableElmt;

void MpmMatchCleanup(MpmThreadCtx *);
MpmMatch *MpmMatchAlloc(MpmThreadCtx *);
int MpmMatchAppend(MpmThreadCtx *, PatternMatcherQueue *, MpmEndMatch *, MpmMatchBucket *, uint16_t, uint16_t);
MpmEndMatch *MpmAllocEndMatch (MpmCtx *);
void MpmEndMatchFreeAll(MpmCtx *mpm_ctx, MpmEndMatch *em);
void MpmMatchFreeSpares(MpmThreadCtx *mpm_ctx, MpmMatch *m);

MpmTableElmt mpm_table[MPM_TABLE_SIZE];
void MpmTableSetup(void);
void MpmRegisterTests(void);

void MpmInitCtx (MpmCtx *mpm_ctx, uint16_t matcher);

#endif /* __UTIL_MPM_H__ */

