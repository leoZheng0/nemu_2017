#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

//返回一个新的监视点
WP* new_wp(){
  if(free_==NULL){
    return NULL; 
  }
  WP* ori = free_;
  free_ = free_->next;//这个时候free_可能为NULL
  ori->next = head;//所以head的末尾一定是NULL
  head = ori;
  return ori;
}

void free_wp(WP *wp){
  WP* pri = NULL;
  for(int i=0;i<NR_WP;i++){
    if(wp_pool[i].next==wp){
      pri = &wp_pool[i];
      break;
    }
  }
  if(pri!=NULL){
    //有前向指针
    pri->next = wp->next;
  }
  if(head == wp){
    //如果是头指针
    head = wp->next;
  }
  wp->next = free_;
  free_ = wp;
}

void print_w() {
	WP *h = head;
	while (h != NULL) {
		printf("[Watchpoint NO.%d]\tExpression: %s\tValue: %d\n", h -> NO, h -> exprs, h -> val);
		h = h -> next;
	}
}

void check_wp(bool* stop) {
	WP* h = head;
	while (h != NULL) {
		bool tmp = true;
		uint32_t cur_val = expr(h->exprs, &tmp);
		if (cur_val != h -> val) {
			printf("[Watchpoint NO.%d]\tExpression: %s\tOrigin Value: %d\tNew Value: %d\n", h -> NO, h -> exprs, h -> val, cur_val);
			h -> val = cur_val;
			*stop = true;
		}
		h = h -> next;
	}
}

//根据no 获得wp
WP* get_wp(int no,bool *find){
  WP* ret = head;
	while (ret != NULL && ret -> NO != no) {
		ret = ret -> next;
	}
	if (ret == NULL) *find = false;
	return ret;
}