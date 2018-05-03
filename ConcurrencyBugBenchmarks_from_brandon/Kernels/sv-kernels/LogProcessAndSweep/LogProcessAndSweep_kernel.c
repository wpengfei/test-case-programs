#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define LOGSIZE 4096 



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
  int buf[LOGSIZE];
  int *next_byte;
  pthread_mutex_t *acc_lock;
} buggy_log;


int io_bound_data_processing(int i){
        int j;
        for(j = 0; j < 100; j++){}
	return i/2;
}

void *change_data_in_log(void *log_p){
	buggy_log *log = (buggy_log *) log_p;
	int log_entry; int *this_byte; int i;

	for(i = 0; i < 1000; i++){
	  int z;
		pthread_mutex_lock(log->acc_lock);
                INSTRUMENT_OFF();NOOP(
                int tmp = execCount;
                )INSTRUMENT_ON();
		log_entry = *(log->next_byte);	
		this_byte = log->next_byte;	
   	        pthread_mutex_unlock(log->acc_lock);
		
		log_entry = io_bound_data_processing(log_entry);
	
		pthread_mutex_lock(log->acc_lock);	
                INSTRUMENT_OFF();NOOP(
                if(tmp != execCount){
                  bugs++;
                } 
                )INSTRUMENT_ON();
		if(log->next_byte == this_byte){
			*(log->next_byte) = log_entry;	
		}else{
			pthread_mutex_unlock(log->acc_lock);
			continue;
		}	
			
		log->next_byte + 1 >= log->buf + LOGSIZE ? log->next_byte =
			log->buf : log->next_byte++;
		pthread_mutex_unlock(log->acc_lock);	
	}
	
}


void *write_out_log(void *log_p){
	buggy_log *log = (buggy_log *)log_p;
	int i; 
	int j;
	for(j = 0; j < 500; j++){
		pthread_mutex_lock(log->acc_lock);
		/*Sweep Conservatively*/
		for(i = 0; i < log->next_byte - log->buf; i++){
			log->buf[i]=rand();
		} 
                INSTRUMENT_OFF();NOOP(
                execCount++; 
                )INSTRUMENT_ON();
		pthread_mutex_unlock(log->acc_lock);
	}
	return (NULL);
}


//Given a queue
buggy_log l;


int main(int argc, char ** argv){
srand(1000);
//add junk so it is there to play with
int i;
for(i=0; i<LOGSIZE; i++){
	l.buf[i]=rand();
}
l.next_byte = l.buf;
l.acc_lock = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
pthread_mutex_init(l.acc_lock,NULL);

pthread_t t1, t2, t3, t4, t5;

pthread_create(&t1,NULL,change_data_in_log,&l);

pthread_create(&t2,NULL,change_data_in_log,&l);

pthread_create(&t3,NULL,change_data_in_log,&l);

pthread_create(&t5,NULL,change_data_in_log,&l);

pthread_create(&t4,NULL,write_out_log,&l);

pthread_join(t1,NULL);
pthread_join(t2,NULL);
pthread_join(t3,NULL);
pthread_join(t4,NULL);
pthread_join(t5,NULL);
  
  INSTRUMENT_OFF();NOOP(
  WRITE_BUGGY();
  )INSTRUMENT_ON();


}



