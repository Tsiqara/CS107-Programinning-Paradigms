#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation)
{
    v->elemSize = elemSize;
    assert(elemSize > 0);
    v->allocLength = initialAllocation;
    v->reallocIncrement = (initialAllocation == 0) ? 10 : initialAllocation;
    v->logLength = 0;
    assert(initialAllocation >= 0);
    v->base = malloc(initialAllocation * elemSize);
    assert(v->base != NULL);
    v->freefn = freeFn;
}

void VectorDispose(vector *v)
{
    for(int i = 0; i < v->logLength; i ++){
        void* elemAdress = (char*)v->base + i * v->elemSize;
        if(v->freefn != NULL){
            v->freefn(elemAdress);
        }
    }
    free(v->base);
}

int VectorLength(const vector *v)
{ return v->logLength; }

void *VectorNth(const vector *v, int position)
{ 
    assert(position >= 0 && position <= v->logLength - 1);

    void* res = (void*)((char*)v->base + position * v->elemSize);
    return res;
}

void VectorReplace(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0 && position <= v->logLength - 1);

    void* addr = (void*)((char*)v->base + position * v->elemSize);
    if(v->freefn != NULL){
        v->freefn(addr);
    }
    memcpy(addr, elemAddr, v->elemSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0 && position <= v->logLength);

    if(v->logLength == v->allocLength){
        v->base = realloc(v->base, (v->allocLength + v->reallocIncrement) * v->elemSize);
        assert(v->base != NULL);
        v->allocLength += v->reallocIncrement;
    }

    void* addr = (void*)((char*)v->base + position * v->elemSize);
    if(position != v->logLength){
        memmove((void*)((char*)addr + v->elemSize), addr, (v->logLength - position) * v->elemSize);
    }
    memcpy(addr, elemAddr, v->elemSize);
    v->logLength ++;
}

void VectorAppend(vector *v, const void *elemAddr)
{
    if(v->logLength == v->allocLength){
        v->base = realloc(v->base, (v->allocLength + v->reallocIncrement) * v->elemSize);
        assert(v->base != NULL);
        v->allocLength += v->reallocIncrement;  
    }

    void* dest = (void*)((char*)v->base + v->logLength * v->elemSize);
    memcpy(dest, elemAddr, v->elemSize);
    v->logLength ++;
}

void VectorDelete(vector *v, int position)
{
    assert(position >= 0 && position <= v->logLength - 1);

    void* addr = VectorNth(v,position);
    if(v->freefn != NULL){
        v->freefn(addr);
    }

    if(position != v->logLength - 1){
        memmove(addr, (void*)((char*)addr + v->elemSize), (v->logLength - position) * v->elemSize);
    }
    v->logLength --;
}

void VectorSort(vector *v, VectorCompareFunction compare)
{
    assert(compare != NULL);
    qsort(v->base, v->logLength, v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData)
{
    assert(mapFn != NULL);

    for(int i = 0; i < v->logLength; i ++){
        mapFn((void*)((char*)v->base + i * v->elemSize), auxData);
    }
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted)
{ 
    assert(startIndex >= 0 && startIndex <= v->logLength);
    assert(searchFn != NULL);
    assert(key != NULL);

    void* elem;
    if(isSorted){
        elem = bsearch(key, (void*)((char*)v->base + startIndex * v->elemSize), v->logLength - startIndex, v->elemSize, searchFn);
    }else{
        size_t numEl = v->logLength - startIndex;
        elem = lfind(key, (void*)((char*)v->base + startIndex * v->elemSize), &numEl, v->elemSize, searchFn);
    }

    if(elem == NULL){
        return kNotFound;
    }

    return ((char*)elem - (char*)v->base) / v->elemSize;
} 
