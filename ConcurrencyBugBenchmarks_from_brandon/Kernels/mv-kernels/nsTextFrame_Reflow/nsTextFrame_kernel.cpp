#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>

#define NUM_STRS 100
#define DEFAULT_STR_LEN 130

int interleaver_executions;
int bugs;
pthread_mutex_t av_lock;

void INSTRUMENT_ON(){}
void INSTRUMENT_OFF(){}
void WRITE_BUGGY(){
  char *bd = getenv("BUGGYFILE");
  FILE*f=fopen(bd,"w");
  fprintf(f,"%d",bugs);
  fclose(f);
}

class nsCOMPtr_nsITextContent{
public:
  char* content;
  
  nsCOMPtr_nsITextContent(){
    content = (char*)malloc(128*sizeof(char));
    for(int i = 0; i < 128; i++){
      content[i] = (i % 26) + 65; //A-Z
    }
  }

  void GetText(const char* c){
    c = content; 
  }
};

class nsIContent{
public:
  nsCOMPtr_nsITextContent textContent;  
};


class nsTextFrame{
public:
  bool mState;
  int mContentOffset;
  int mContentLength;
  int mColumn;
  nsIContent*      mContent;

  nsTextFrame(){
    mContent = new nsIContent();
  }

  void
  PaintAsciiText();

  bool 
  Reflow();


  nsCOMPtr_nsITextContent 
  do_QueryInterface(nsIContent* Content){
    //This normally goes to a UI/external interface
    mContentOffset = rand() % 64;
    mContentLength = rand() % 64;
    
    return Content->textContent;      
  }
  
};

class nsTextFragment{
public:
  char *buffer;
  char *get1b(bool t){
    if(!buffer && t){
      buffer = (char*)malloc(sizeof(char)*DEFAULT_STR_LEN); 
    }
    return buffer;
  } 
  
  nsTextFragment(){
  }
};

void
nsTextFrame::PaintAsciiText()
{
  // Get the text fragment
  nsCOMPtr_nsITextContent tc = do_QueryInterface(mContent);
  ////////////////////////////////////
  //           \/--mContentOffset
  //frag-->|XX|Th|is|is|th|eg|oo|te|xt|XX|XX|
  //           mContentLength = 16
  ////////////////////////////////////
  nsTextFragment* frag = new nsTextFragment();
  tc.GetText(frag->get1b(false));
  

  // See if we need to transform the text. If the text fragment is ascii and
  // wasn't transformed, then we can skip this step. If we're displaying the
  // selection and the text is selected, then we need to do this step so we
  // can create the index buffer
  int textLength = 0;
  const char* text;
  int tmp;
    //(mContentOffset + mContentLength <= frag->GetLength())
    //BRANDON: THESE ACCESSES SHOULD BE ATOMIC W/ ONE ANOTHER
    pthread_mutex_lock(&av_lock);
    INSTRUMENT_OFF();NOOP(
    tmp=interleaver_executions);
    INSTRUMENT_ON();
    text = frag->get1b(false) + mContentOffset;
    pthread_mutex_unlock(&av_lock);
    //mContentOffset changes in another thread
    //mContentLength changes in another thread
    pthread_mutex_lock(&av_lock);
    textLength = mContentLength;

    INSTRUMENT_OFF();NOOP(
    if(tmp != interleaver_executions){ bugs++; }
    )INSTRUMENT_ON();

    pthread_mutex_unlock(&av_lock);
    //Now text is the old text pointer which is old mContentLength chars from
    //the end of the buffer, and textLength is new mContentLength from the
    //end of the buffer.  They are (potentially) inconsistent.  Reflow, when
    //called asynchronously causes this to happen (it interleaves wherever it
    //wants to


    // See if the text ends in a newline
    // BRANDON: Here is the MANIFESTATION POINT.  If they're out of sync
    // the deref of text with textLength, when textLength is too big
    // breaks things.  Instead, we'll do a check that tells us if 
    // we would segfault at the dereference
    char* cp = frag->get1b(false) + 128;

    /*
    if ((textLength > 0) && (text[textLength - 1] == '\n')) {
      textLength--;
    }
    NS_ASSERTION(textLength >= 0, "bad text length");
    */
//Questionable Block of Code End
}

class TextData{
public:
  int mOffset;

  TextData(){
    mOffset = rand() % 128;
  } 
   
  int getStartingOffset(){
    int rlen = mOffset - (rand() % 64);
  }

};

bool 
nsTextFrame::Reflow()
{

  //BRANDON: This can intervene between the other accesses to the 
  //text in the objects here.  mContentLength is set here, and can
  //render the length inconsistent with recently updated text:
  //ie: text = blah; mContentOffset = startingOffset; mcontentLength = textData.mOffset - startingOffset; textLength = blah; (races with the one in Paint).
  // Set content offset and length
  //
  TextData td = TextData();
  int startingOffset = td.getStartingOffset(); 
  
  //BRANDON: THESE ACCESSES SHOULD BE ATOMIC W/ ONE ANOTHER
  pthread_mutex_lock(&av_lock);

  mContentOffset = startingOffset;
  mContentLength = td.mOffset - startingOffset;

  INSTRUMENT_OFF();NOOP(
  interleaver_executions++;
  )INSTRUMENT_ON();
  pthread_mutex_unlock(&av_lock);
  
  return true;
}

void *paint_repeatedly(void*v){
  nsTextFrame* n = (nsTextFrame*)v;
  for(int i = 0; i < 1000; i ++){
    n->PaintAsciiText(); 
    for(int j = 0; j < 500; j++){}    
  } 
}

void *reflow_repeatedly(void*v){
  nsTextFrame* n = (nsTextFrame*)v;
  for(int i = 0; i < 10; i ++){
    n->Reflow(); 
    for(int j = 0; j < 1000; j++){}    
  } 
}


int main(int argc, char** argv){

  nsTextFrame *nstf = new nsTextFrame();
  pthread_mutex_init(&av_lock,NULL);
  pthread_t paint, reflow;
  pthread_create(&paint,NULL,paint_repeatedly,(void*)nstf);
  pthread_create(&reflow,NULL,reflow_repeatedly,(void*)nstf);

  pthread_join(paint,NULL);
  pthread_join(reflow,NULL);
  INSTRUMENT_OFF();NOOP(
  WRITE_BUGGY();
  )INSTRUMENT_ON();


}
