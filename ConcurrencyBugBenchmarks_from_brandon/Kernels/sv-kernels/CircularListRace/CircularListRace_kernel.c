#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define LISTSIZE 10000

int bugs;
int execCount;

void INSTRUMENT_OFF(){}
void INSTRUMENT_ON(){}
void WRITE_BUGGY(){
  char *bd = getenv("BUGGYFILE");
  FILE*f=fopen(bd,"w");
  fprintf(f,"%d",bugs);
  fclose(f);
}


typedef struct {
  int buf[LISTSIZE];
  int head, tail, len, in_use;
  pthread_mutex_t *mut;
} list;



int getFromTail(list *l){
	int t;		
	if(l->len == 0)
		return -1000;

	t = l->buf[l->tail];

	l->len = (l->len == 0) ? 0 : l->len--;
	
	l->tail = (l->tail - 1) % LISTSIZE;

}

void addAtHead(list *l,int n){
	int i;

	l->tail = (l->tail+1 % LISTSIZE);

	l->len++;

	for(i=l->head; i != l->tail; i=((i+1)%LISTSIZE))
		l->buf[(i+1)%LISTSIZE] = l->buf[i];

	l->buf[l->head] = n;
}

void *tail_to_head(void *ll){
	int n;
	list *l = (list *)ll;
	/*We need to make the block delimited by NonAtomicBegin() and End()

	  the atomic part!*/
	
	int i,j;
	for(i=0;i<50;i++){
        
	  pthread_mutex_lock(l->mut);
          INSTRUMENT_OFF();NOOP(
          int tmp = execCount;	
          )INSTRUMENT_ON();	
	  n = getFromTail(l);
          pthread_mutex_unlock(l->mut);
	
	/*Do some other stuff in here and forget that you wanted this atomic
	  or just decide locking twice is more fun, or something*/
	/*THIS IS WHERE THE OTHER THREAD CAN WEDGE ITSELF IN*/	

	//	for(j=0; j < 100; j++);		
	
	  pthread_mutex_lock(l->mut);
          INSTRUMENT_OFF();NOOP(
          if(tmp != execCount){
            bugs++;
          }
          execCount++;
          )INSTRUMENT_ON();	
	  addAtHead(l,n);
          pthread_mutex_unlock(l->mut);
	}

	return (NULL);
	/*Down to here!*/
}


list l;

int main(int argc, char ** argv){
  
  //add junk so it is there to play with
  int i;
  for(i=0; i<LISTSIZE; i++){
  	l.buf[i]=i;
  }
  
  l.head=0; 
  l.tail=LISTSIZE-1; 
  l.len=LISTSIZE; 
  l.in_use = 0;
  l.mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init(l.mut,NULL);
  
  pthread_t t1, t2;
  
  pthread_create(&t1,NULL,tail_to_head,&l);
  pthread_create(&t2,NULL,tail_to_head,&l);
  
  pthread_join(t1,NULL);
  pthread_join(t2,NULL);

  INSTRUMENT_OFF();NOOP(
  WRITE_BUGGY();
  )INSTRUMENT_ON();

}

