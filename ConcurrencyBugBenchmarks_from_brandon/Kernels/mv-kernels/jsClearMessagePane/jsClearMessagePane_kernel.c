#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>

void INSTRUMENT_ON(){}
void INSTRUMENT_OFF(){}

#ifndef NONBUG_SPIN_ITERS
#define NONBUG_SPIN_ITERS 5000
#endif

#ifndef NUM_BUG_ITERS
#define NUM_BUG_ITERS 10000
#endif

#define HIDDEN 5 
#define PAGE_LEN 512
#define MAX_FOLDER_TREE_ITEMS 128
#define MAX_FRAMES 32
#define ACCOUNT_CENTRAL_PANE 0 
#define NUM_PAGES 64
#define THREAD_PANE_SIZE 256
#define MESSAGE_PANE_SIZE 512
#define NUM_MSGS_BOX_ATTRS 16
#define NUM_AC_BOX_ATTRS 16
#define ACCT_CTRL_INDEX 4
int *gAccountCentralLoaded; //"bool"
char *messagesBox[NUM_MSGS_BOX_ATTRS];
char *accountCentralBox[NUM_AC_BOX_ATTRS];
char *threadPane;
char *messagePane;
pthread_mutex_t *gAccountCentralLoaded_Lock;

//Instrumentation counters, these are not real code.
int read_loaded;
int bugs;

void WRITE_BUGGY(){
  char *bd = getenv("BUGGYFILE");
  FILE*f=fopen(bd,"w");
  fprintf(f,"%d",bugs);
  fclose(f);
}

typedef struct _Page{
  char * url;
  char *content; 
}Page;

void init_page(Page *page,const char *url,const char *content){
 
  page->url = (char*)malloc((strlen(url)+2)*sizeof(char));
  strcpy(page->url,url);
  
  page->content = (char*)malloc((strlen(content)+2)*sizeof(char));
  strcpy(page->content,content);
}

typedef struct _PageList{
  int num_pages;
  Page *pages[NUM_PAGES];
}PageList;
PageList *pageList;//needs initialization -- done

typedef struct _Frame{
  Page * location;
}Frame;

typedef struct _Window{
  Frame * frames[MAX_FRAMES];  
}Window;
Window *window;//needs initilaization -- done

typedef struct _FolderTreeItem{
  int itemcontents;   
}FolderTreeItem;

typedef struct _FolderTreeItemList{
  FolderTreeItem *items[MAX_FOLDER_TREE_ITEMS]; 
  int length;
}FolderTreeItemList;

typedef struct _FolderTree{
  FolderTreeItemList * selectedItems;
}FolderTree;
FolderTree *folderTree;//needs initialization

FolderTree * GetFolderTree(){
  return folderTree;
}

void ClearThreadPane(){
  memset(threadPane,0,THREAD_PANE_SIZE);
}

void ClearMessagePane(){
  memset(messagePane,0,MESSAGE_PANE_SIZE);
}

void ChangeFolderByDOMNode(FolderTreeItem *fti){
  //no op?
}
void FolderPaneSelectionChange(){
  FolderTree* tree = GetFolderTree();
  if(tree){
    FolderTreeItemList * selArray = tree->selectedItems;
    if( selArray && (selArray->length == 1) ){
      ChangeFolderByDOMNode(selArray->items[0]);
    } else {
      ClearThreadPane();
    }
  }
  pthread_mutex_lock(gAccountCentralLoaded_Lock);
  if(!(*gAccountCentralLoaded)){

    INSTRUMENT_OFF();NOOP(
    read_loaded++;
    )INSTRUMENT_ON();

    pthread_mutex_unlock(gAccountCentralLoaded_Lock);
    ClearMessagePane();
    return;
  }

  INSTRUMENT_OFF();NOOP(
  read_loaded++; 
  )INSTRUMENT_ON();

  pthread_mutex_unlock(gAccountCentralLoaded_Lock);
  return;
}


Page *pref_getLocalizedUnicharPref(const char *page){
  if(page){
    int i; 
    for(i = 0; i < NUM_PAGES; i++){
      if(pageList->pages[i]->url && !strcmp(pageList->pages[i]->url,page)){
        return pageList->pages[i];
      }
    }
  }
}


void messagesBox_setAttribute(int attr,const char*val){
  if(messagesBox[attr]){
    free(messagesBox[attr]);
  }
  messagesBox[attr] = (char *)malloc((strlen(val)+2)*sizeof(char));//room for null
  strcpy(messagesBox[attr], val);
}

void messagesBox_removeAttribute(int attr,const char*val){
  if(messagesBox[attr]){
    free(messagesBox[attr]);
    messagesBox[attr] = 0;
  }
}

void accountCentralBox_setAttribute(int attr,const char*val){
  if(accountCentralBox[attr]){
    free(accountCentralBox[attr]);
  }
  accountCentralBox[attr] = (char *)malloc((strlen(val)+2)*sizeof(char));//room for null
  strcpy(accountCentralBox[attr], val);
}

void accountCentralBox_removeAttribute(int attr){
  if(accountCentralBox[attr]){
    free(accountCentralBox[attr]);
    accountCentralBox[attr] = 0;    
  } 
}

void ShowAccountCentral()
{
  Page * acctCentralPage = pref_getLocalizedUnicharPref("mailnews.account_central_page.url");
  messagesBox_setAttribute(HIDDEN, "true");
  accountCentralBox_removeAttribute(HIDDEN);
  //Here, window->...->location is set to acctCentralPage 
  pthread_mutex_lock(gAccountCentralLoaded_Lock);
 
  INSTRUMENT_OFF();NOOP(
  //read_loaded is a counter of times interleaving code exec.
  int tmp = read_loaded; 
  )INSTRUMENT_ON();

  window->frames[ACCOUNT_CENTRAL_PANE]->location = acctCentralPage;
  
  pthread_mutex_unlock(gAccountCentralLoaded_Lock);
  
  
  //Elsewhere, the flag gAccountCentralLoaded may be read before it
  //is updated here to correspond to the acctCentralPage having 
  //been loaded into window->...->location   This is the bug --
  //the flag is stale right here, but is being read.


  pthread_mutex_lock(gAccountCentralLoaded_Lock);

//TODO: make this an #ifdef, so it goes away if user wants  
  INSTRUMENT_OFF();NOOP(
  if(read_loaded != tmp){
    bugs++;
  }
  )INSTRUMENT_ON();

  //Here, the related flag variable gAccountCentralLoaded is set to true
  *gAccountCentralLoaded = 1;
  pthread_mutex_unlock(gAccountCentralLoaded_Lock);

   
}

void *repeatedly_show_account_central(void *v){
  int i;
  for(i = 0; i < 5000; i++){
    ShowAccountCentral();    
    int j; 
    for(j = 0; j < 5000; j++){}
  }
  return NULL;
} 

void *repeatedly_change_folder_pane_selection(void*v){
  int i;
  for(i = 0; i < 70; i++){
    FolderPaneSelectionChange();    
    int j;  
    for(j = 0; j < 1000; j++){}
  }
  return NULL;
}

int main(int argc, char ** argv){
  //Initialize our elaborate data structures...
  threadPane = (char*)malloc(THREAD_PANE_SIZE*sizeof(char));
  messagePane = (char*)malloc(MESSAGE_PANE_SIZE*sizeof(char));
  pageList = (PageList*)malloc(sizeof(PageList));
  pageList->num_pages = NUM_PAGES;
  int i;
  for(i = 0; i < pageList->num_pages; i++){
    pageList->pages[i] = (Page*)malloc(sizeof(Page));
    if(i == ACCT_CTRL_INDEX){
      init_page(pageList->pages[i],"mailnews.account_central_page.url","Account Central:\nHere Are Lots of Options and Probably Some HTML\n\n"); 
    }else if(i == 0){
      init_page(pageList->pages[i],"Starter Page","This Frame hasn't yet been pointed at a valid page."); 
    }else{
      init_page(pageList->pages[i],"no url","<empty page>"); 
    }
  }
  gAccountCentralLoaded = (int*)malloc(sizeof(int));
  (*gAccountCentralLoaded) = 0;

  window = (Window *)malloc(sizeof(Window));
  for(i = 0; i < MAX_FRAMES; i++){
    window->frames[i] = (Frame*)malloc(sizeof(Frame));
    window->frames[i]->location = pageList->pages[0];//default to the empty page
  } 
  folderTree = (FolderTree *)malloc(sizeof(FolderTree));
  folderTree->selectedItems = (FolderTreeItemList*)malloc(sizeof(FolderTreeItemList));
  for(i = 0; i < MAX_FOLDER_TREE_ITEMS; i++){
    folderTree->selectedItems->items[0] = (FolderTreeItem*)malloc(sizeof(FolderTreeItem));
    folderTree->selectedItems->length++;
  }
  for(i = 0; i < NUM_MSGS_BOX_ATTRS; i++){
    messagesBox[i] = 0;
  }
  for(i = 0; i < NUM_AC_BOX_ATTRS; i++){
    accountCentralBox[i] = 0;
  }

  //setup our pthreads junk
  //
  gAccountCentralLoaded_Lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(gAccountCentralLoaded_Lock,NULL);
  pthread_t sac,fpsc;
  pthread_create(&sac,NULL,repeatedly_show_account_central,NULL);
  pthread_create(&fpsc,NULL,repeatedly_change_folder_pane_selection,NULL);

  pthread_join(sac,NULL);
  pthread_join(fpsc,NULL);

  INSTRUMENT_OFF();NOOP( 
  WRITE_BUGGY(); 
  )INSTRUMENT_ON();
}
