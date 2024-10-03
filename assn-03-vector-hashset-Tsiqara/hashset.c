#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn)
{
	assert(elemSize > 0);
	h->elemSize = elemSize;
	assert(numBuckets > 0);
	h->numBuckets = numBuckets;
	h->numElems = 0;
	assert(hashfn != NULL);
	h->hashfn = hashfn;
	assert(comparefn != NULL);
	h->comparefn = comparefn;
	h->freefn = freefn;

	h->buckets = malloc(h->numBuckets * sizeof(vector));
	for(int i = 0; i < numBuckets; i ++){
		VectorNew(&h->buckets[i], h->elemSize, h->freefn, 0);
	}
}

void HashSetDispose(hashset *h)
{
	for(int i = 0; i < h->numBuckets; i ++){
		VectorDispose(&h->buckets[i]);
	}

	free((void*)h->buckets);
	h->numElems = 0;
}

int HashSetCount(const hashset *h)
{ return h->numElems; }

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData)
{
	assert(mapfn != NULL);
	for(int i = 0; i < h->numBuckets; i ++){
		VectorMap(&h->buckets[i], mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr)
{
	assert(elemAddr != NULL);
	int hashCode = h->hashfn(elemAddr, h->numBuckets);
	assert(hashCode >= 0 && hashCode < h->numBuckets);
	int ind = VectorSearch(&h->buckets[hashCode], elemAddr, h->comparefn, 0, false);
	if(ind == -1){
		VectorAppend(&h->buckets[hashCode], elemAddr);
	}else{
		VectorReplace(&h->buckets[hashCode], elemAddr, ind);
	}
	h->numElems ++;
}

void *HashSetLookup(const hashset *h, const void *elemAddr)
{
	assert(elemAddr != NULL);
	int hashCode = h->hashfn(elemAddr, h->numBuckets);
	assert(hashCode >= 0 && hashCode < h->numBuckets);
	int ind = VectorSearch(&h->buckets[hashCode], elemAddr, h->comparefn, 0, false);
	if(ind == -1){
		return NULL;
	}
	return VectorNth(&h->buckets[hashCode], ind);
}
