#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct node{
  unsigned long long int val;
  int visit;
  struct node* next;
};

struct hash{
  struct node **list;
};

//Methods
int isPOT(int n);
int search(struct hash *hsh, unsigned long long int index, unsigned long long int tag);
int searchDirect(struct hash *hsh, unsigned long long int index, unsigned long long int tag);
void insert(struct hash *hsh, unsigned long long int index, unsigned long long int tag);
void insertDirect(struct hash *hsh, unsigned long long int index, unsigned long long int tag);
int countNode(struct hash *hsh, unsigned long long int index);
void lruR(struct hash* hsh, unsigned long long int index, unsigned long long int tag);

//Main
int main(int argc, char** argv){
  if(argc != 7){
    printf("Error not enough args");
    return 0;
  }
  //cache size
  int csize = atoi(argv[1]);
  //check if the first argument is a power of 2
  if(isPOT(csize) == 0){
    printf("Not Power of 2");
    return 0;
  }
  int asso;
  //1 = direct, 2 = fully assoc, >2  = n-way assoc
  if(argv[2][0] == 'd'){
    asso = 1;
  }
  else if(argv[2][5] != ':'){
    asso = 2;
  }
  else{
    sscanf(argv[2],"assoc:%d",&asso);
    
    if(isPOT(asso) == 0){
      printf("Assoc is Not Power of 2");
      return 0;
    }
  }
  //check if assoc is power of 2
  
  //prefetching policy; 0 = p0, 1 = p1
  int pref;
  if(argv[3][1] == '0'){
    pref = 0;
  }
  else{
    pref = 1;
  }
  /*replacement policy; 1 fifo, 2 = lru
  int rep;
  if(argv[4][0] == 'f'){
    rep = 0;
  }
  else{
    rep = 1;
  }*/
  
  //block size
  int bsize = atoi(argv[5]);
  if(isPOT(bsize) == 0){
    printf("Bsize is not Power of 2");
    return 0;
  }
  //read file
  FILE* fp = fopen(argv[6], "r");
  if(fp == NULL){
    return 0;
  }
  int nlines = csize/bsize;
  int mReads=0;
  int mWrites=0;
  int cHits=0;
  int cMiss=0;
  //the setbits
  char oper;
  char add1[20];
  unsigned long long int addy;
  //direct------------------------------------------------------------------ 
  if(asso == 1){
    //printf("in direct without pref\n");
    int nsets;
    //int nindexpre;
    int nindexbits;
    //int nblockpre;
    int nblock;
    
    nsets = nlines;
    //nindexpre = (log(nsets));
    nindexbits = (log(nsets)/log(2));
    //nblockpre = (log(bsize));
    nblock = (log(bsize)/log(2));
    //cache
    struct hash *hsh = (struct hash*)malloc(sizeof(struct hash));
    hsh->list = (struct node**)malloc(nsets*sizeof(struct node*));
    int x;
    for(x = 0; x < nsets; x++){
      hsh->list[x] = (struct node*)malloc(asso*sizeof(struct node));
      hsh->list[x] = NULL;
    }
    while(!feof(fp)){
      fscanf(fp,"%s ", add1);
      if(strcmp(add1, "#eof") == 0){
	break;
      }
      fscanf(fp, "%c %llx", &oper, &addy);
      
      unsigned long long int setnum = addy>>nblock;
      unsigned long long int tag = addy>>(nindexbits + nblock);
      unsigned long long int x = ((1<<nindexbits)-1);
      unsigned long long int index = setnum&x;

      unsigned long long int addyP = addy + bsize;
      unsigned long long int tagP = addyP>>(nindexbits + nblock);
      unsigned long long int setnumP = addyP>>nblock;
      unsigned long long int indexP = setnumP&((1<<nindexbits)-1);

      //printf("copied vars\n");
      if(oper == 'R'){
	//printf("in read\n");
	int y = searchDirect(hsh, index, tag);
	//printf("finish search\n");
	if(y == -1){
	  if(countNode(hsh, index) == 1){
	    hsh->list[index]->val = tag;
	  }
	  else{
	    insertDirect(hsh, index, tag);
	  }
	  if(pref == 1){
	    if(searchDirect(hsh,indexP, tagP) == -1){
	      if(countNode(hsh, indexP) == 1){
		hsh->list[indexP]->val = tagP;
		mReads++;
	      }
	      else{
		insertDirect(hsh, indexP, tagP);
		mReads++;
	      }
	    }
	  }
	  cMiss++;
	  mReads++;
	}
	else if(y == 1){
	  
	  cHits++;
	}
      }
      if(oper == 'W'){
	int y = searchDirect(hsh, index, tag);
	if(y == -1){
	  cMiss++;
	  mReads++;
	  if(countNode(hsh, index) == 1){
	    hsh->list[index]->val = tag;
	  }
	  else{
	  insertDirect(hsh, index, tag);
	  }
	  if(pref == 1){
	    if(searchDirect(hsh,indexP, tagP) == -1){
	      if(countNode(hsh, indexP) == 1){
		hsh->list[indexP]->val = tagP;
		mReads++;
	      }
	      else{
		insertDirect(hsh, indexP, tagP);
		mReads++;
	      }
	    }
	  }
	  mWrites++;
	}
	if(y == 1){
	  
	  mWrites++;
	  cHits++;
	}
      }
    }
  }
  //full associativity--------------------------------------------------------------------
  if(asso == 2){
    int nsets = 1;
    //int nindexpre = (log(nsets));
    int nindexbits = (log(nsets)/log(2));
    //int nblockpre = (log(bsize));
    int nblock = (log(bsize)/log(2));
    int count;
    struct hash* hsh = (struct hash*)malloc(sizeof(struct hash));
    hsh->list = (struct node**)malloc(nsets*sizeof(struct node*));

    hsh->list[0] = (struct node*)malloc(nlines*sizeof(struct node));
    hsh->list[0] = NULL;
    while(!feof(fp)){
      fscanf(fp,"%s ", add1);
      if(strcmp(add1, "#eof") == 0){
	break;
      }
      fscanf(fp, "%c %llx", &oper, &addy);
      
      unsigned long long int setnum = addy>>nblock;
      unsigned long long int tag = addy>>(nindexbits + nblock);
      unsigned long long int x = ((1<<nindexbits)-1);
      unsigned long long int index = setnum&x;
      index = 0;
      
      //unsigned long long int addyP = addy + bsize;
      //unsigned long long int tagP = addyP>>(nindexbits + nblock);
      //unsigned long long int setnumP = addyP>>nblock;
      //unsigned long long int indexP = setnumP&((1<<nindexbits)-1);
      
      if(oper == 'R'){
	int y = search(hsh, index, tag); 
	if(y == -1){
	  mReads++;
	  insert(hsh, index, tag);
	  if(pref == 1){
	    count = countNode(hsh, index);
	    // && hsh->list[0]->next != NULL
	    if(count > nlines){
	      struct node *ptr;
	      ptr = hsh->list[index];
	      struct node *prev;
	      while(ptr->next != NULL){
		prev = ptr;
		ptr = ptr->next;
	      }
	      prev->next = NULL;
	    }
	    if(search(hsh, index, tag+1) == -1){
	      insert(hsh, index, tag+1);
	      mReads++;
	    }
	  }
	  cMiss++;
	}
	if(y == 1){
	  
	  cHits++;
	}
      }
      if(oper == 'W'){
	int y = search(hsh, index, tag);
	if(y == -1){
	  cMiss++;
	  mReads++;
	  insert(hsh, index, tag);
	  if(pref == 1){
	    count = countNode(hsh, index);
	    if(count > nlines){
	      struct node *ptr;
	      ptr = hsh->list[index];
	      struct node *prev;
	      while(ptr->next != NULL){
		prev = ptr;
		ptr = ptr->next;
	      }
	      prev->next = NULL;
	    }
	    if(search(hsh, index, tag+1) == -1){
	      insert(hsh, index, tag+1);
	      mReads++;
	    }
	  }
	  mWrites++;
	}
	if(y == 1){
	  
	  cHits++;
	  mWrites++;
	}
      }
      //fifo
      count = countNode(hsh, index);
      if(count > nlines){
	struct node *ptr;
	ptr = hsh->list[index];
	struct node *prev;
	while(ptr->next != NULL){
	  prev = ptr;
	  ptr = ptr->next;
	}
	prev->next = NULL;
      }
    }
  }
  //N-Associativity------------------------------------------------
  if(asso > 2){
    int nsets = nlines/asso;
    //int nindexpre = (log(nsets));
    int nindexbits = (log(nsets)/log(2));
    //int nblockpre = (log(bsize));
    int nblock = (log(bsize)/log(2));
    int count;
    
    struct hash *hsh = (struct hash*)malloc(sizeof(struct hash));
    hsh->list = (struct node**)malloc(nsets*sizeof(struct node*));
    int x;
    for(x = 0; x < asso; x++){
      hsh->list[x] = (struct node*)malloc(asso*sizeof(struct node));
      hsh->list[x] = NULL;
    }

    while(!feof(fp)){
      fscanf(fp,"%s ", add1);
      if(strcmp(add1, "#eof") == 0){
	break;
      }
      fscanf(fp, "%c %llx", &oper, &addy);
      
      unsigned long long int setnum = addy>>nblock;
      unsigned long long int tag = addy>>(nindexbits + nblock);
      unsigned long long int x = ((1<<nindexbits)-1);
      unsigned long long int index = setnum&x;
      
      unsigned long long int addyP = addy + bsize;
      unsigned long long int tagP = addyP>>(nindexbits + nblock);
      unsigned long long int setnumP = addyP>>nblock;
      unsigned long long int indexP = setnumP&((1<<nindexbits)-1);
      if(oper == 'R'){
	int y = search(hsh, index, tag); 
	if(y == -1){
	  mReads++;
	  cMiss++;
	  insert(hsh, index, tag);
	  if(pref == 1){
	    if(search(hsh, indexP, tagP) == -1){
	      insert(hsh, indexP, tagP);
	      mReads++;
	    }
	  }
	}
	if(y == 1){
	  
	  cHits++;
	}
      }
      if(oper == 'W'){
	int y = search(hsh, index, tag);
	if(y == -1){
	  cMiss++;
	  mReads++;
	  insert(hsh, index, tag);
	  if(pref == 1){
	    if(search(hsh, indexP, tagP) == -1){
	      insert(hsh, indexP, tagP);
	      mReads++;
	    }
	  }
	  mWrites++;
	}
	if(y == 1){
	  
	  cHits++;
	  mWrites++;
	}
      }
      count = countNode(hsh, index);
      if(count>asso){
	struct node *ptr;
	ptr = hsh->list[index];
	struct node *prev;
	while(ptr->next != NULL){
	  prev = ptr;
	  ptr = ptr->next;
	}
	prev->next = NULL;
      }
      if(pref == 1){
	count = countNode(hsh, indexP);
	if(count > asso){
	  struct node *ptr;
	  ptr = hsh->list[indexP];
	  struct node *prev;
	  while(ptr->next != NULL){
	    prev = ptr;
	    ptr = ptr->next;
	  }
	  prev->next = NULL;
	}
      }
    }
  }
  printf("Memory reads: %d\n", mReads);
  printf("Memory writes: %d\n", mWrites);
  printf("Cache hits: %d\n", cHits);
  printf("Cache misses: %d\n", cMiss);

  return 0;
}
//-----------------------------------------------------------------------End of Main
//Power of Two
int isPOT(int n){
  //printf("In POT\n");
  if(n == 0){
    return 0;
  }
  while(n != 1){
    if(n % 2 != 0){
      return 0;
    }
    n = n/2;
  }
  return 1;
}
//countNode
int countNode(struct hash* hsh, unsigned long long int index){
  //printf("In countNode\n");
  int count = 0;
  struct node *ptr;
  ptr = hsh->list[index];
  while(ptr!=NULL){
    count++;
    ptr = ptr->next;
  }
  return count;
}

//Search
int search(struct hash *hsh, unsigned long long int index, unsigned long long int tag){
  //printf("In Search\n");
  if(hsh->list[index] == NULL){
    return -1;
  }
  struct node *ptr = (struct node*)malloc(sizeof(struct node));
  ptr = hsh->list[index];
  while(ptr!=NULL){
    if(ptr->val == tag){
      return 1;
    }
    ptr = ptr->next;
  }
  return -1;
}

int searchDirect(struct hash *hsh, unsigned long long int index, unsigned long long int tag){
  //printf("In Search Direct\n");
  if(hsh->list[index] == NULL){
    return -1;
  }
  struct node *ptr = (struct node*)malloc(sizeof(struct node));
  ptr = hsh->list[index];
  if(ptr->val == tag){
    return 1;
  }
  return -1;
}
  
//Insert for regular Hashtable
void insert(struct hash *hsh, unsigned long long int index, unsigned long long int tag){
  //printf("In Insert\n");
  struct node *nNode = (struct node*)malloc(sizeof(struct node));
  nNode->val = tag;
  nNode->visit = 0;
  nNode->next = NULL;
  if(hsh->list[index] == NULL){
    hsh->list[index] = nNode;
    return;
  }
  else{
    struct node *ft = hsh->list[index];
    nNode->next = ft;
    ft = nNode;
    hsh->list[index] = ft;
    return;
  }
}
void insertDirect(struct hash* hsh, unsigned long long int index, unsigned long long int tag){
  //printf("In Insert Direct\n");
  struct node *nNode = (struct node*)malloc(sizeof(struct node));
  nNode->val = tag;
  nNode->visit = 0;
  nNode->next = NULL;
  hsh->list[index] = nNode;
  return;
}
