#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int bugs;
int execCount;

void INSTRUMENT_ON(){}
void INSTRUMENT_OFF(){}
void WRITE_BUGGY(){
  char *bd = getenv("BUGGYFILE");
  FILE*f=fopen(bd,"w");
  fprintf(f,"%d",bugs);
  fclose(f);
}

typedef struct {
  char *buf;
  int len;
  pthread_mutex_t *mut;
} StringBuffer;

StringBuffer *sb;
StringBuffer *sb2;


StringBuffer *new_StringBuffer(const char *str,int len){
  StringBuffer * sb = (StringBuffer *)malloc(sizeof(StringBuffer));
  sb->buf = (char *)malloc(sizeof(char) * len);
  sb->len = len; 
  sb->mut = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(sb->mut,NULL);

  memcpy(sb->buf,str,len);
  return sb;
}

int SB_length(StringBuffer *sb){
  if(sb == NULL) return -1;
  return sb->len;
}

void SB_expandCapacity(StringBuffer *sb,int newLen){
  sb->buf = (char *)realloc(sb->buf,newLen);
}

void SB_getChars(StringBuffer *sb, int start, int end, char *dst, int dstBegin){
  int copyLen;
  copyLen = end - start;
  
  if(start+copyLen > sb->len)
    copyLen = sb->len - start;  
 
  int dstAllocLen;
  dstAllocLen = dstBegin + copyLen;  
  /*always do this safely, even though it is slow.*/ 
  dst = realloc(dst,dstAllocLen);
  memcpy(dst+dstBegin,sb->buf+start,copyLen);
} 

StringBuffer *SB_append(StringBuffer *this,StringBuffer *sb){
  pthread_mutex_lock(this->mut);
  pthread_mutex_lock(sb->mut);
  INSTRUMENT_OFF();NOOP(
  int tmp = execCount;
  )INSTRUMENT_ON();
  int len = SB_length(sb);
  int newcount = this->len + len;
  if(newcount > this->len) SB_expandCapacity(this, newcount);
  pthread_mutex_unlock(sb->mut);
  pthread_mutex_unlock(this->mut);
  

  pthread_mutex_lock(this->mut);
  pthread_mutex_lock(sb->mut);
  SB_getChars(sb,0,len,this->buf,this->len);
  this->len = newcount;
  INSTRUMENT_OFF();NOOP(
  if(tmp != execCount){
    bugs++;
  }
  execCount++;
  )INSTRUMENT_ON();
  pthread_mutex_unlock(sb->mut);
  pthread_mutex_unlock(this->mut);
  return this;
}

void *append_repeatedly(void*p){
  int i; 
  for(i = 0; i < 20; i ++){
    SB_append(sb,sb2);
  }
}

int main(int argc, char *argv){
  srand(time(0));
  pthread_t t1,t2;
  
  sb = new_StringBuffer("Finkleroy",strlen("Finkleroy"));
  sb2 = new_StringBuffer("FinkleroyRoyRoy",strlen("FinkleroyRoyRoy"));
  
  pthread_create(&t1,NULL,append_repeatedly,(NULL));
  pthread_create(&t2,NULL,append_repeatedly,(NULL));
  
  pthread_join(t1,NULL);  
  pthread_join(t2,NULL);  

  INSTRUMENT_OFF();NOOP(
  WRITE_BUGGY();  
  )INSTRUMENT_ON();

}
