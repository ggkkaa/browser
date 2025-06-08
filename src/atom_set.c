#include "atom_set.h"
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define ATOM_SET_BUCKET_ALLOC malloc
#define ATOM_SET_ALLOC malloc
#define ATOM_SET_DEALLOC(ptr, n) free(ptr)

bool atom_set_reserve(AtomTable* set, size_t extra) {
    if(set->len + extra > set->buckets.len) {
        size_t ncap = set->buckets.len*2 + extra;
        AtomTableBucket** newbuckets = ATOM_SET_ALLOC(sizeof(*newbuckets)*ncap);
        if(!newbuckets) return false;
        memset(newbuckets, 0, sizeof(*newbuckets) * ncap);
        for(size_t i = 0; i < set->buckets.len; ++i) {
            AtomTableBucket* oldbucket = set->buckets.items[i];
            while(oldbucket) {
                AtomTableBucket* next = oldbucket->next;
                size_t hash = ((size_t)oldbucket->atom) % ncap;
                AtomTableBucket* newbucket = newbuckets[hash];
                oldbucket->next = newbucket;
                newbuckets[hash] = oldbucket;
                oldbucket = next;
            }
        }
        ATOM_SET_DEALLOC(set->buckets.items, set->buckets.cap * sizeof(*set->buckets.items));
        set->buckets.items = newbuckets;
        set->buckets.len = ncap;
    }
    return true;
}
bool atom_set_insert(AtomSet* set, Atom* atom) {
    if(!atom_set_reserve(set, 1)) return false;
    size_t hash = ((uintptr_t)atom) % set->buckets.len;
    AtomTableBucket* into = set->buckets.items[hash];
    AtomTableBucket* bucket = ATOM_SET_BUCKET_ALLOC(sizeof(AtomTableBucket));
    if(!bucket) return false;
    bucket->next = into;
    bucket->atom = atom;
    set->buckets.items[hash] = bucket;
    set->len++;
    return bucket;
}
bool atom_set_get(AtomSet* set, Atom* atom) {
    if(set->len == 0) return false;
    assert(set->buckets.len > 0);
    size_t hash = ((uintptr_t)atom) % set->buckets.len;
    AtomTableBucket* bucket = set->buckets.items[hash];
    while(bucket) {
        if(bucket->atom == atom) return bucket->atom;
        bucket = bucket->next;
    }
    return false;
}
