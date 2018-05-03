#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define PROPERTY_CACHE_SIZE 512
#define JS_TRUE 1
#define JS_FALSE 0

void INSTRUMENT_ON(){}
void INSTRUMENT_OFF(){}
int numbugs;
void WRITE_BUGGY(){
  char *bd = getenv("BUGGYFILE");
  FILE*f=fopen(bd,"w");
  fprintf(f,"%d",numbugs);
  fclose(f);
}

pthread_mutex_t cache_lock;
typedef unsigned uint32;
typedef struct _JSProperty {
    unsigned int id;
}JSProperty;

typedef struct _JSObjectMap {
    unsigned int nrefs;          /* count of all referencing objects */
    //JSObjectOps *ops;           /* high level object operation vtable */
    unsigned int nslots;         /* length of obj->slots vector */
    unsigned int freeslot;       /* index of next free obj->slots element */
} JSObjectMap;

typedef struct _JSObject {
    JSObjectMap *map;
    void        *slots;
} JSObject;


typedef struct _JSPropertyCacheEntry {
    struct {
	JSObject    *object;    /* weak link to object */
	JSProperty  *property;  /* weak link to property, or not-found id */
    } s;
} JSPropertyCacheEntry;


typedef struct _JSPropertyCache {
    JSPropertyCacheEntry table[PROPERTY_CACHE_SIZE];
    int                  empty;
    uint32               fills;
    uint32               recycles;
    uint32               tests;
    uint32               misses;
    uint32               flushes;
} JSPropertyCache;

typedef struct _JSRuntime {
  JSPropertyCache *propertyCache;
} JSRuntime;

typedef struct _JSContext {
  JSRuntime *runtime;
} JSContext;

void NonAtomicBegin(){}
void NonAtomicEnd(){}

void
js_FlushPropertyCache(JSContext *cx)
{
    JSPropertyCache *cache;

    pthread_mutex_lock(&cache_lock);
    cache = cx->runtime->propertyCache;
    if (cache->empty){
        pthread_mutex_unlock(&cache_lock);
        return;
    }
    memset(cache->table, 0, sizeof cache->table);
    cache->empty = JS_TRUE;
    cache->flushes++;
    pthread_mutex_unlock(&cache_lock);
}

#define PROPERTY_CACHE_HASH(obj, id) id

void 
js_PropertyCacheFill(JSPropertyCache *cache,JSObject *obj,int id,JSProperty *prop){
  int NUMFLUSHES_BEGIN;
  unsigned int _hashIndex = (unsigned int)PROPERTY_CACHE_HASH(obj, id);
  JSPropertyCache *_cache = (cache);                                    
  JSPropertyCacheEntry *_pce = &(_cache->table[_hashIndex]);              
  JSProperty *_pce_prop;                                                
  //This is normally performed w/ ATOMIC_DWORD_STORE
  //but in this kernel, it suffices to ensure that the store
  //of object and property take place in a locked region
  //
  //The bug is right here:
  //These two stores to pce->s result in something being in the cache
  //The flag _cache->empty needs to be updated at the same time
  //but the two updates occur in separate atomic operations, instead
  //of the same one.
  INSTRUMENT_OFF();NOOP(
  NUMFLUSHES_BEGIN = 0;
  )INSTRUMENT_ON();

  pthread_mutex_lock(&cache_lock);
  _pce->s.object = obj;                                             
  _pce->s.property = prop;                                          

  INSTRUMENT_OFF();NOOP(
  NUMFLUSHES_BEGIN = _cache->flushes;
  )INSTRUMENT_ON();

  pthread_mutex_unlock(&cache_lock);
  //Flush can happen in here. 
  
  //This is also done with an atomic store operation  
  pthread_mutex_lock(&cache_lock);
  //The bug is that this assignment should be in the same crit.
  //sec. as the store to object and property
  INSTRUMENT_OFF();NOOP(
  if(_cache->flushes > NUMFLUSHES_BEGIN){
    numbugs++;
  }
  )INSTRUMENT_ON();

  _cache->empty = JS_FALSE;                                             
  pthread_mutex_unlock(&cache_lock);

  _cache->fills++;                                                      
}

JSObject *get_new_object(){
  JSObjectMap *newmap = (JSObjectMap*)malloc(sizeof(JSObjectMap));
  newmap->nrefs = newmap->nslots = newmap->freeslot = 0;
  JSObject * newobj = (JSObject*)malloc(sizeof(JSObject));
  newobj->map = newmap;
  newobj->slots = NULL;
  return newobj;
}

JSProperty *get_new_property(){
  JSProperty * newprop = (JSProperty*)malloc(sizeof(JSProperty));
  newprop->id = rand() % PROPERTY_CACHE_SIZE;
  return newprop;
}
void *continuously_fill_cache(void*v){
  JSContext*ctx = (JSContext*)v;
  int i;
  for(i = 0; i < 1000; i ++){
    //create new JSObject
    JSObject *jso = get_new_object();
    //create new JSProperty
    JSProperty *jsp = get_new_property();
    //get new id
    int id = rand() % PROPERTY_CACHE_SIZE;
    //call cache fill
    js_PropertyCacheFill(ctx->runtime->propertyCache,
                         jso, id, jsp);
    int j; 
  }
}



void *continuously_flush_cache(void*v){
  JSContext *ctx = (JSContext*)v;
  int i = 0;
  for(i = 0; i < 100; i++){
    js_FlushPropertyCache(ctx); 
  }
    int j; 
}

int main(int argc, char**argv){
  pthread_mutex_init(&cache_lock,NULL);

  pthread_t fill;
  pthread_t flush;

  JSContext *ctx = (JSContext*)malloc(sizeof(JSContext));
  ctx->runtime = (JSRuntime *)malloc(sizeof(JSRuntime));
  ctx->runtime->propertyCache = (JSPropertyCache*)malloc(sizeof(JSPropertyCache));
  ctx->runtime->propertyCache->empty = 0;
  ctx->runtime->propertyCache->fills = 0;
  ctx->runtime->propertyCache->recycles = 0;
  ctx->runtime->propertyCache->tests = 0;
  ctx->runtime->propertyCache->misses = 0;
  ctx->runtime->propertyCache->flushes = 0;
 
  pthread_create(&fill,NULL,continuously_fill_cache,(void*)ctx);  
  pthread_create(&flush,NULL,continuously_flush_cache,(void*)ctx);  

  pthread_join(fill,NULL);
  pthread_join(flush,NULL); 


  INSTRUMENT_OFF();NOOP(
  WRITE_BUGGY(); 
  )INSTRUMENT_ON();
}
