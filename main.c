#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "list.h"
#include "hash.h"
#include "bitmap.h"

#define MAX_DS   10
#define MAX_NAME 64
#define MAX_LINE 1024
#define MAXTOK  6
#define UNUSED(x) ((void)(x))

//list와 hash에 쓸 구조체
struct list_item {
    struct list_elem elem;
    int data;
};
struct hash_item {
    struct hash_elem hash_elem;
    int data;
};

#define hash_entry(HASH_ELEM, STRUCT, MEMBER) ((STRUCT *)((uint8_t *)(HASH_ELEM) - offsetof(STRUCT, MEMBER)))


//유니온으로 사용하는 자료구조 하나로 합치기
enum ds_type { DS_NONE=0, DS_LIST, DS_HASH, DS_BITMAP };

struct ds_entry {
    char name[MAX_NAME];
    enum ds_type type;
    union {
        struct list *list;
        struct hash *hash;
        struct bitmap *bm;
    };
};
static struct ds_entry pool[MAX_DS];


// 이름으로 찾기
static int find_ds(const char *name){
    int ret = -1;
    int i=0;
    while(i<MAX_DS){
        int used= (pool[i].type!=DS_NONE);
        if(used){
            int ok = (strcmp(pool[i].name,name)==0);
            if(ok){
                ret=i;
                break;
            }
        }
        i++;
    }
    return ret;
}

// 빈슬롯 할당
static int alloc_slot(const char *name, enum ds_type t){
    int pos = -1;
    for(int i=0;i<MAX_DS;i++){
        int empty=(pool[i].type==DS_NONE);
        if(empty){
            pos=i;
            break;
        }
    }
    if(pos>=0){
        strncpy(pool[pos].name, name, MAX_NAME-1);
        pool[pos].name[MAX_NAME-1]='\0';
        pool[pos].type = t;
    }
    return pos;
}


//쓰여야 하는 함수들 모음
static bool cmp_list(const struct list_elem *a,const struct list_elem *b, void *aux){
    UNUSED(aux);
    int va = list_entry(a, struct list_item, elem)->data;
    int vb = list_entry(b,struct list_item,elem)->data;
    if(va<vb) return true;
    return false;
}

static unsigned hash_func(const struct hash_elem *e,void *aux){
    UNUSED(aux);
    struct hash_item *tmp = hash_entry(e,struct hash_item,hash_elem);
    int v=tmp->data;
    unsigned hv = hash_int(v);
    return hv;
}
static bool hash_cmp(const struct hash_elem *a, const struct hash_elem *b,void *aux){
    UNUSED(aux);
    struct hash_item *x = hash_entry(a, struct hash_item, hash_elem);
    struct hash_item *y = hash_entry(b,struct hash_item,hash_elem);
    if(x->data < y->data) return true;
    else return false;
}

static void act_square(struct hash_elem *e, void *aux){
    UNUSED(aux);
    struct hash_item *p=hash_entry(e,struct hash_item,hash_elem);
    int d=p->data;
    p->data = d*d;
}
static void act_triple(struct hash_elem *e,void *aux){
    UNUSED(aux);
    struct hash_item *p = hash_entry(e, struct hash_item, hash_elem);
    int d = p->data;
    int r = d*d*d;
    p->data=r;
}

static void act_free(struct hash_elem *e,void *aux){
    UNUSED(aux);
    struct hash_item *tmp=hash_entry(e,struct hash_item,hash_elem);
    free(tmp);
}


// n번째 elem을 가져오기
static struct list_elem *get_nth(struct list *l, int n){
    struct list_elem *cur = list_begin(l);
    int i=0;
    while(i<n){
        if(cur==list_end(l)) break;
        cur = list_next(cur);
        i++;
    }
    return cur;
}
static void free_listall(struct list *l){
    struct list_elem *e = list_begin(l);
    while(e!=list_end(l)){
        struct list_elem *nxt=list_next(e);
        struct list_item *tmp = list_entry(e,struct list_item,elem);
        list_remove(e);
        free(tmp);
        e=nxt;
    }
}

// ~~~.c에서 작성한 함수들 메인에 선언하기
void list_swap(struct list_elem *a, struct list_elem *b);
void list_shuffle(struct list *list);
unsigned hash_int_2(int i);
struct bitmap *bitmap_expand(struct bitmap *bitmap,int size);


//입력된 문자열을 띄어쓰기 기준으로 나눠주는 역할
static int tokenize(char *line,char *tok[]){
    int c=0;
    char *p = strtok(line," \t");
    while(p!=NULL && c<MAXTOK){
        tok[c]=p;
        c++;
        p=strtok(NULL," \t");
    }
    return c;
}

//list item을 생성
static struct list_item *mkitem(int v){
    struct list_item *p = malloc(sizeof(struct list_item));
    p->data=v;
    return p;
}

// list 개별 핸들러 함수들임
static void lcmd_push_front(char *tok[],int n,struct list *l){
    UNUSED(n);
    struct list_item *it = mkitem(atoi(tok[2]));
    list_push_front(l,&it->elem);
}
static void lcmd_push_back(char *tok[],int n,struct list *l){
    UNUSED(n);
    struct list_item *it=mkitem(atoi(tok[2]));
    list_push_back(l, &it->elem);
}
static void lcmd_size(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    printf("%zu\n",list_size(l));
}
static void lcmd_empty(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    int r=list_empty(l);
    if(r) printf("true\n");
    else printf("false\n");
}
static void lcmd_pop_front(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    if(list_empty(l)) return;
    struct list_elem *e=list_pop_front(l);
    struct list_item *tmp=list_entry(e,struct list_item,elem);
    free(tmp);
}
static void lcmd_pop_back(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    int emp = list_empty(l);
    if(emp) return;
    struct list_elem *e = list_pop_back(l);
    free(list_entry(e,struct list_item,elem));
}
static void lcmd_front(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    if(list_empty(l)) return;
    struct list_elem *e = list_front(l);
    printf("%d\n", list_entry(e, struct list_item,elem)->data);
}
static void lcmd_back(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    if(list_empty(l)) return;
    int v = list_entry(list_back(l),struct list_item,elem)->data;
    printf("%d\n",v);
}
static void lcmd_insert(char *tok[],int n,struct list *l){
    UNUSED(n);
    int p=atoi(tok[2]);
    int v=atoi(tok[3]);
    struct list_item *it = mkitem(v);
    list_insert(get_nth(l,p), &it->elem);
}
static void lcmd_insert_ordered(char *tok[],int n,struct list *l){
    UNUSED(n);
    struct list_item *it = mkitem(atoi(tok[2]));
    list_insert_ordered(l, &it->elem,cmp_list,NULL);
}
static void lcmd_remove(char *tok[],int n,struct list *l){
    UNUSED(n);
    int p = atoi(tok[2]);
    struct list_elem *e=get_nth(l,p);
    if(e!=list_end(l)){
        list_remove(e);
        struct list_item *tmp=list_entry(e,struct list_item,elem);
        free(tmp);
    }
}
static void lcmd_swap(char *tok[],int n,struct list *l){
    UNUSED(n);
    int idx[]={atoi(tok[2]),atoi(tok[3])};
    struct list_elem *elems[]={get_nth(l,idx[0]),get_nth(l,idx[1])};
    if(elems[0]!=list_end(l)){
        if(elems[1]!=list_end(l))
            list_swap(elems[0],elems[1]);
    }
}
static void lcmd_reverse(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    list_reverse(l);
}
static void lcmd_splice(char *tok[],int n,struct list *l){
    UNUSED(n);
    int args[]={atoi(tok[2]),atoi(tok[4]),atoi(tok[5])};
    int di=find_ds(tok[3]);
    if(di<0) return;
    if(pool[di].type!=DS_LIST) return;
    struct list *l2=pool[di].list;
    struct list_elem *pts[]={get_nth(l,args[0]),get_nth(l2,args[1]),get_nth(l2,args[2])};
    if(pts[1]!=list_end(l2))
        list_splice(pts[0],pts[1],pts[2]);
}
static void lcmd_sort(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    list_sort(l,cmp_list,NULL);
}
static void lcmd_shuffle(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    list_shuffle(l);
}
static void lcmd_unique(char *tok[],int n,struct list *l){
    struct list *dup=NULL;
    if(n>=3){
        int di = find_ds(tok[2]);
        if(di>=0){
            if(pool[di].type==DS_LIST)
                dup=pool[di].list;
        }
    }
    list_unique(l,dup,cmp_list,NULL);
}
static void lcmd_max(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    if(list_empty(l)) return;
    struct list_elem *e=list_max(l,cmp_list,NULL);
    struct list_item *it=list_entry(e,struct list_item,elem);
    printf("%d\n",it->data);
}
static void lcmd_min(char *tok[],int n,struct list *l){
    UNUSED(n); UNUSED(tok);
    if(list_empty(l)) return;
    struct list_elem *mx = list_min(l,cmp_list,NULL);
    printf("%d\n", list_entry(mx,struct list_item,elem)->data);
}

static void handle_list_cmd(char *cmd,char *tok[],int n,struct list *l){
    char *sub=cmd+5;

    if(strcmp(sub,"push_front")==0){ lcmd_push_front(tok,n,l); return; }
    if(strcmp(sub,"push_back")==0){ lcmd_push_back(tok,n,l); return; }
    if(strcmp(sub,"size")==0){ lcmd_size(tok,n,l); return; }
    if(strcmp(sub,"empty")==0){ lcmd_empty(tok,n,l); return; }
    if(strcmp(sub,"pop_front")==0){ lcmd_pop_front(tok,n,l); return; }
    if(strcmp(sub,"pop_back")==0){ lcmd_pop_back(tok,n,l); return; }
    if(strcmp(sub,"front")==0){ lcmd_front(tok,n,l); return; }
    if(strcmp(sub,"back")==0){ lcmd_back(tok,n,l); return; }
    if(strcmp(sub,"insert_ordered")==0){ lcmd_insert_ordered(tok,n,l); return; }
    if(strcmp(sub,"insert")==0){ lcmd_insert(tok,n,l); return; }
    if(strcmp(sub,"remove")==0){ lcmd_remove(tok,n,l); return; }
    if(strcmp(sub,"swap")==0){ lcmd_swap(tok,n,l); return; }
    if(strcmp(sub,"reverse")==0){ lcmd_reverse(tok,n,l); return; }
    if(strcmp(sub,"splice")==0){ lcmd_splice(tok,n,l); return; }
    if(strcmp(sub,"sort")==0){ lcmd_sort(tok,n,l); return; }
    if(strcmp(sub,"shuffle")==0){ lcmd_shuffle(tok,n,l); return; }
    if(strcmp(sub,"unique")==0){ lcmd_unique(tok,n,l); return; }
    if(strcmp(sub,"max")==0){ lcmd_max(tok,n,l); return; }
    if(strcmp(sub,"min")==0){ lcmd_min(tok,n,l); return; }
}

//cmd에 쓰이는 애들 판별
static void handle_hash_cmd(char *cmd, char *tok[], int n, struct hash *h);
static void handle_bm_cmd(char *cmd,char *tok[],int n,int idx);


//create 해주는 함수
static void do_create(char *tok[],int n){
    if(n<3) return;

    enum ds_type tp;
    if(strcmp(tok[1],"list")==0) tp=DS_LIST;
    else if(strcmp(tok[1],"hashtable")==0) tp = DS_HASH;
    else tp=DS_BITMAP;

    int s = alloc_slot(tok[2],tp);
    if(s<0) return;

    //list인지 해쉬인지 비트맵인지에 따라서 다르게
    switch(tp){
        case DS_LIST:
            pool[s].list=malloc(sizeof(struct list));
            list_init(pool[s].list);
            break;
        case DS_HASH:
            pool[s].hash = malloc(sizeof(struct hash));
            hash_init(pool[s].hash,hash_func,hash_cmp,NULL);
            break;
        case DS_BITMAP:{
            size_t bcnt = (size_t)atoi(tok[3]);
            pool[s].bm=bitmap_create(bcnt);
            break;
        }
        default: break;
    }
}

//delete하는 함수
static void do_delete(char *tok[]){
    int i=find_ds(tok[1]);
    if(i<0) return;
    if(pool[i].type==DS_LIST){
        free_listall(pool[i].list);
        free(pool[i].list);
    }else if(pool[i].type==DS_HASH){
        hash_destroy(pool[i].hash,act_free);
        free(pool[i].hash);
    }else{
        bitmap_destroy(pool[i].bm);
    }
    pool[i].type=DS_NONE;
    pool[i].name[0]='\0';
}


//dumpdata 실행해주는 함수
static void do_dump(char *tok[]){
    int i = find_ds(tok[1]);
    if(i<0) return;
    int tp=pool[i].type;

    if(tp==DS_LIST){
        struct list *l=pool[i].list;
        if(list_empty(l)) return;

        struct list_elem *e=list_begin(l);
        int cnt=0;
        while(e != list_end(l)){
            if(cnt>0) printf(" ");
            int v=list_entry(e,struct list_item,elem)->data;
            printf("%d",v);
            cnt++;
            e = list_next(e);
        }
        printf("\n");
    }
    else if(tp==DS_HASH){
        struct hash *h = pool[i].hash;
        int emp = hash_empty(h);
        if(emp) return;

        struct hash_iterator it;
        hash_first(&it, h);
        struct hash_elem *he;
        while(1){
            he = hash_next(&it);
            if(he==NULL) break;
            struct hash_item *tmp = hash_entry(he,struct hash_item,hash_elem);
            printf("%d ", tmp->data);
        }
        printf("\n");
    }
    else if(tp==DS_BITMAP){
        struct bitmap *b = pool[i].bm;
        size_t sz=bitmap_size(b);
         size_t j=0;
        while(j<sz){
            int bit=bitmap_test(b,j);
            if(bit) putchar('1');
            else putchar('0');
            j++;
        }
        putchar('\n');
    }
}

//명령어별 커맨드 모아놓은 함수
static void process_command(char *line){
    char *tok[MAXTOK]={NULL};
    int n=tokenize(line,tok);
    if(n<=0) return;

    char *cmd=tok[0];
    int idx;

    if(strcmp(cmd,"quit")==0) exit(0);
    if(strcmp(cmd,"create")==0){ do_create(tok,n); return; }
    if(strcmp(cmd, "delete")==0){ do_delete(tok); return; }
    if(strcmp(cmd,"dumpdata")==0){ do_dump(tok); return;}

    // list
    if(strncmp(cmd,"list_",5)==0){
        idx=find_ds(tok[1]);
        if(idx<0) return;
        if(pool[idx].type!=DS_LIST) return;
        handle_list_cmd(cmd,tok,n,pool[idx].list);
        return;
    }
    //해쉬
    if(strncmp(cmd, "hash_",5)==0){
        idx = find_ds(tok[1]);
        if(idx<0) return;
        if(pool[idx].type != DS_HASH) return;
        handle_hash_cmd(cmd,tok,n,pool[idx].hash);
        return;
    }

    //비트맵
    if(strncmp(cmd,"bitmap_",7)==0){
        idx = find_ds(tok[1]);
        if(idx<0) return;
        if(pool[idx].type!=DS_BITMAP) return;
        handle_bm_cmd(cmd,tok,n,idx);
    }
}




//해쉬 item 생성
static struct hash_item *mkhash(int v){
    struct hash_item *p=malloc(sizeof(struct hash_item));
    p->data = v;
    return p;
}

//해쉬 관련 명령어들 처리해주는 함수
static void handle_hash_cmd(char *cmd, char *tok[], int n, struct hash *h){
    UNUSED(n);
    char *sub = cmd+5;

    if(strcmp(sub,"insert")==0){
        int v = atoi(tok[2]);
        struct hash_item *it=mkhash(v);
        struct hash_elem *old=hash_insert(h,&it->hash_elem);
        if(old!=NULL){
            free(it);
        }
        return;
    }

    if(strcmp(sub,"replace")==0){
        struct hash_item *it = mkhash(atoi(tok[2]));
        struct hash_elem *old = hash_replace(h,&it->hash_elem);
        if(old!=NULL){
            struct hash_item *prev=hash_entry(old,struct hash_item,hash_elem);
            free(prev);
        }
        return;
    }
     if(strcmp(sub, "find")==0){
        struct hash_item key;
        key.data=atoi(tok[2]);
        struct hash_elem *f = hash_find(h, &key.hash_elem);
        if(f==NULL) return;
        struct hash_item *r=hash_entry(f,struct hash_item,hash_elem);
        printf("%d\n",r->data);


        return;
    }

    if(strcmp(sub,"delete")==0){
        struct hash_item key;
        key.data = atoi(tok[2]);
        struct hash_elem *d=hash_delete(h,&key.hash_elem);
        if(d==NULL) return;
        free(hash_entry(d, struct hash_item, hash_elem));
        return;
    }
    if(strcmp(sub,"apply")==0){
        char *m=tok[2];
        if(strcmp(m,"square")==0)
            hash_apply(h,act_square);
        else if(strcmp(m,"triple")==0)
            hash_apply(h, act_triple);
        return;
    }
    if(strcmp(sub,"size")==0){
        size_t s = hash_size(h);
        printf("%zu\n",s);
        return;
    }
    if(strcmp(sub,"empty")==0){
        if(hash_empty(h)) printf("true\n");
        else printf("false\n");
        return;
    }
    if(strcmp(sub,"clear")==0){
        hash_clear(h,act_free);
        return;
    }
}


// true인지 체크
static bool chktrue(char *s){
    if(s==NULL) return false;
    return strcmp(s,"true")==0;
}


//비트맵 관련 커맨드 함수들
static void handle_bm_cmd(char *cmd,char *tok[],int n,int idx){
    UNUSED(n);
    struct bitmap *bm=pool[idx].bm;
    char *sub=cmd+7;

    if(strcmp(sub,"set")==0){
        size_t p=(size_t)atoi(tok[2]);
        bool v = chktrue(tok[3]);
        bitmap_set(bm,p,v);
        return;
    }
     if(strcmp(sub,"mark")==0){
        bitmap_mark(bm,(size_t)atoi(tok[2]));
        return;
    }

    if(strcmp(sub,"reset")==0){
        size_t p = (size_t)atoi(tok[2]);
        bitmap_reset(bm, p);
        return;
    }
    if(strcmp(sub,"flip")==0){
        bitmap_flip(bm, (size_t)atoi(tok[2]));
        return;
    }
    if(strcmp(sub,"test")==0){
        size_t p=(size_t)atoi(tok[2]);
        int r = bitmap_test(bm,p);
        if(r) printf("true\n");
        else printf("false\n");
        return;
    }

    if(strcmp(sub,"set_all")==0){
        bitmap_set_all(bm, chktrue(tok[2]));
        return;
    }
    if(strcmp(sub,"set_multiple")==0){
        size_t st=(size_t)atoi(tok[2]);
        size_t c = (size_t)atoi(tok[3]);
        bool v=chktrue(tok[4]);
        bitmap_set_multiple(bm,st,c,v);
        return;
    }

    if(strcmp(sub,"count")==0){
        size_t st = (size_t)atoi(tok[2]);
        size_t c=(size_t)atoi(tok[3]);
        bool v = chktrue(tok[4]);
        printf("%zu\n",bitmap_count(bm,st,c,v));
        return;
    }
    if(strcmp(sub,"contains")==0){
        size_t range[]={(size_t)atoi(tok[2]),(size_t)atoi(tok[3])};
        bool v=chktrue(tok[4]);
        int r=bitmap_contains(bm,range[0],range[1],v);
        if(r) printf("true\n");
        else printf("false\n");
        return;
    }

    if(strcmp(sub,"any")==0){
        size_t st = (size_t)atoi(tok[2]);
        size_t c = (size_t)atoi(tok[3]);
        if(bitmap_any(bm,st,c)) printf("true\n");
        else printf("false\n");
        return;
    }
    if(strcmp(sub,"none")==0){
        size_t st=(size_t)atoi(tok[2]),c=(size_t)atoi(tok[3]);
        int r = bitmap_none(bm,st,c);
        printf("%s\n", r?"true":"false");
        return;
    }
    if(strcmp(sub,"all")==0){
        size_t st = (size_t)atoi(tok[2]);
        size_t c=(size_t)atoi(tok[3]);
        int r=bitmap_all(bm,st,c);
        if(r) printf("true\n");
        else printf("false\n");
        return;
    }

    if(strcmp(sub,"scan")==0){
            size_t st=(size_t)atoi(tok[2]);
        size_t c = (size_t)atoi(tok[3]);
        bool v=chktrue(tok[4]);
        size_t r = bitmap_scan(bm,st,c,v);
        printf("%zu\n",r);
        return;
    }
    if(strcmp(sub,"scan_and_flip")==0){
        size_t args[]={(size_t)atoi(tok[2]),(size_t)atoi(tok[3])};
        bool v = chktrue(tok[4]);
        printf("%zu\n", bitmap_scan_and_flip(bm,args[0],args[1],v));
        return;
    }
    if(strcmp(sub,"size")==0){
        printf("%zu\n",bitmap_size(bm));
        return;
    }

    if(strcmp(sub,"dump")==0){
        bitmap_dump(bm);
        return;
    }
    if(strcmp(sub,"expand")==0){
        int ns = atoi(tok[2]);
        struct bitmap *nb = bitmap_expand(bm,ns);
        if(nb!=NULL)
            pool[idx].bm=nb;
        return;
    }
}


int main(void){
    char line[MAX_LINE];
    memset(pool,0,sizeof(pool));

    while(fgets(line,sizeof(line),stdin)!=NULL){
        size_t len=strlen(line);
        if(len>0 && line[len-1]=='\n')
            line[len-1]='\0';

         if(strlen(line)==0) continue;

        if(line[0]=='['){
            int rep=0,off=0;
            if(sscanf(line,"[x%d] %n",&rep,&off)==1){
                char *sc = line+off;
                for(int i=0;i<rep;i++){
                    char cp[MAX_LINE];
                    strncpy(cp,sc,MAX_LINE-1);
                    cp[MAX_LINE-1]='\0';
                    process_command(cp);
                }
            }
        }
        else{
            process_command(line);
        }
    }
    return 0;
}
