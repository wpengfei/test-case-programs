#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#define NUM_STRINGS 100

int interleaver_executions;
int bugs;

void INSTRUMENT_ON(){}
void INSTRUMENT_OFF(){}
void WRITE_BUGGY(){
  char *bd = getenv("BUGGYFILE");
  FILE*f=fopen(bd,"w");
  fprintf(f,"%d",bugs);
  fclose(f);
}

pthread_mutex_t *inc_lock;
#define JS_RUNTIME_METER(rt,which) pthread_mutex_lock(inc_lock); \
                                   INSTRUMENT_OFF();NOOP( \
                                   tmp = interleaver_executions; \
                                   )INSTRUMENT_ON(); \
                                   rt->which++; \
                                   pthread_mutex_unlock(inc_lock);

#define JS_LOCK_RUNTIME_VOID(rt,thing1,thing2) pthread_mutex_lock(inc_lock); \
                                               INSTRUMENT_OFF();NOOP( \
                                               if(interleaver_executions != tmp){ bugs++; } \
                                               )INSTRUMENT_ON(); \
                                               thing1; \
                                               thing2; \
                                               pthread_mutex_unlock(inc_lock);

typedef int uint32;
typedef int uintN;
typedef int jsrefcount;
typedef int JSPackedBool;
typedef int JSBool;
typedef long long int int64;
typedef double jsdouble;
typedef char jsword;
typedef char jschar;

typedef struct _JSString{
  int length;
  char *chars; 
} JSString;

JSString *stringCache[NUM_STRINGS];

typedef struct _JSRuntime {

    /* Garbage collector state, used by jsgc.c. */
    jsrefcount          gcDisabled;
    uint32              gcBytes;
    uint32              gcLastBytes;
    uint32              gcMaxBytes;
    uint32              gcLevel;
    uint32              gcNumber;
    JSPackedBool        gcPoke;
    JSPackedBool        gcRunning;
    uint32              gcMallocBytes;

    JSBool              rngInitialized;
    int64               rngMultiplier;
    int64               rngAddend;
    int64               rngMask;
    int64               rngSeed;
    jsdouble            rngDscale;

    /* Well-known numbers held for use by this runtime's contexts. */
    jsdouble            *jsNaN;
    jsdouble            *jsNegativeInfinity;
    jsdouble            *jsPositiveInfinity;

    /* Empty string held for use by this runtime's contexts. */
    JSString            *emptyString;

    /* List of active contexts sharing this runtime. */

    /* These are used for debugging -- see jsprvtd.h and jsdbgapi.h. */
    void                *interruptHandlerData;
    void                *newScriptHookData;
    void                *destroyScriptHookData;
    void                *debuggerHandlerData;
    void                *sourceHandlerData;
    void                *executeHookData;
    void                *callHookData;
    void                *objectHookData;
    void                *throwHookData;
    void                *debugErrorHookData;

    /* Client opaque pointer */
    void                *data;

    /* These combine to interlock the GC and new requests. */
    uint32              requestCount;
    jsword              gcThread;

    /* Lock and owning thread pointer for JS_LOCK_RUNTIME. */
    jsword              rtLockOwner;

    /* String instrumentation. */
    jsrefcount          liveStrings;
    jsrefcount          totalStrings;
    double              lengthSum;
    double              lengthSquaredSum;
}JSRuntime; //must be initialized

typedef struct _JSContext {
  JSRuntime *runtime;
} JSContext; //must be initialized

JSString *
js_NewString(JSContext *cx, jschar *chars, size_t length, uintN gcflag)
{
    JSString *str;

    str = (JSString *) malloc(sizeof(JSString));
    if (!str)
        return NULL;
    str->length = length;
    str->chars = chars;
    JSRuntime *rt = cx->runtime;

    INSTRUMENT_OFF();NOOP(
    int tmp = 0;
    )INSTRUMENT_ON();

    JS_RUNTIME_METER(rt, liveStrings);

    //This is the first operation which may be interleaved
    JS_RUNTIME_METER(rt, totalStrings);

    /*
     *If something interleaves here, and reads totalStrings, and
     *wants to also read and use lengthSum or lengthSquaredSum,
     *it will have a new value for totalStrings, and an old value
     *for length*Sum 
    */

    
    //And this is the second operation
    JS_LOCK_RUNTIME_VOID(rt, rt->lengthSum += (double)length,
         rt->lengthSquaredSum += (double)length * (double)length);
    return str;
}


void printJSStringStats(JSRuntime *rt) {
    double mean = 0., var = 0., sigma = 0.;
    //Interleaver
    pthread_mutex_lock(inc_lock);

    INSTRUMENT_OFF();NOOP(
    interleaver_executions++;
    )INSTRUMENT_ON();

    jsrefcount count = rt->totalStrings;
    if (count > 0 && rt->lengthSum >= 0) {
        mean = rt->lengthSum / count;
        var = count * rt->lengthSquaredSum - rt->lengthSum * rt->lengthSum;
        if (var < 0.0 || count <= 1)
            var = 0.0;
        else
            var /= count * (count - 1);

        /* Windows says sqrt(0.0) is "-1.#J" (?!) so we must test. */
        sigma = (var != 0.) ? sqrt(var) : 0.;
    }
    pthread_mutex_unlock(inc_lock);
}

void *repeatedly_print_strings(void*v){
  JSContext* ctx = (JSContext*)v;
  int i;
  for(i = 0; i < 100; i++){
    printJSStringStats(ctx->runtime);
    int j;
    for(j = 0; j < 2000; j++){}
  }
  return NULL;
}

void putString(JSString * newstr,int ind){
  if(stringCache[ind%NUM_STRINGS]){
    free(stringCache[ind%NUM_STRINGS]);
  }
  stringCache[ind%NUM_STRINGS] = newstr;
}

void *repeatedly_make_strings(void*v){
  JSContext* cx = (JSContext*)v;
  int i; 
  for(i = 0; i < 1000; i++){
    //need to determine their length
    int length = 5 + rand() % 100;
    
    //need to make new chars
    char * newchars = (char*)malloc((length + 2)*sizeof(char));
    memset(newchars,0,length+2);
    int j; 
    for(j = 0; j < length; j++){
      newchars[j] = 64 + rand() % 58; //alphabets
    }
    //need to pass nonsense gcflag
    int gcflag = 10 + rand() % 1000;
  
    JSString * newstr = 
               js_NewString(cx, newchars, length, gcflag);

    putString(newstr,i);
    int k;
    for(k = 0; k < 2000; k++){}
  }
  return NULL;
}

int main(int argc, char ** argv){
  JSContext *context = (JSContext*)malloc(sizeof(JSContext));
  memset(context, 0, sizeof(JSContext));
  context->runtime = (JSRuntime*)malloc(sizeof(JSRuntime));
  memset(context->runtime, 0, sizeof(JSRuntime));
  
  inc_lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(inc_lock,NULL);
  
  pthread_t create,print;
  pthread_create(&create,NULL,repeatedly_make_strings,(void*)context);
  pthread_create(&print,NULL,repeatedly_print_strings,(void*)context);

  pthread_join(create,NULL);
  pthread_join(print,NULL);
  
  INSTRUMENT_OFF();NOOP(
  WRITE_BUGGY();
  )INSTRUMENT_ON();

}
