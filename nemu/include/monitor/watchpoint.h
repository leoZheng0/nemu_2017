#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
	uint32_t val;
	char exprs[32];

} WP;
WP* new_wp();
void free_wp(WP *wp);
void print_w();
void check_wp(bool* stop);
WP* get_wp(int no,bool *find);
#endif
