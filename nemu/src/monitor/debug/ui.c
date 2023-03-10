#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#define TEST_VALID(x) if(x){printf("Invalid args!\n");return 0;}

void cpu_exec(uint64_t);


/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");//等待指令

  if (line_read && *line_read) {//如果用户输入了什么,就将指令放到"history"链表的末尾
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

//单步执行
static int cmd_si(char *args){
  char *arg = strtok(NULL, " ");
  int step;
  if(arg == NULL){
    step = 1;
  }
  else{
    sscanf(arg,"%d",&step);
    if(step<=0){
      printf("you can't exec %d step(s), require >= 1 step(s)!\n",step);
      return 0;
    }
  }
  cpu_exec(step);
  return 0;
}

//info
static int cmd_info(char *args){

  char *arg = strtok(NULL, " ");
  TEST_VALID(arg==NULL);

  switch(*arg){
    case 'r':{
      //print reg info
      int i;
      for (i = R_EAX; i <= R_EDI; i ++) {
        printf("$%s\t:0x%08x\n",regsl[i],reg_l(i));
      }
      printf("$eip\t:0x%08x\n",cpu.eip);
      break;
    }
    case 'w':{
      //print watchpoint info
      break;
    }
    default:{
      printf("Unknown arg '%s'\n", arg);
      break;
    }
  }
  return 0;
}

//scan mem
static int cmd_x(char *args){

  char *arg = strtok(NULL, " ");
  TEST_VALID(arg==NULL);
  int N = atoi(arg);

  char* expr_str = "";
  while( (arg = strtok(NULL, " "))!=NULL){
    strcat(expr_str,arg);
  }
  strcat(expr_str,"\0");
  TEST_VALID(expr_str==NULL);

  //这里expr应该返回一个uint32类型的数值,因为地址一定为正
  bool success;
  uint32_t base = expr(expr_str,&success);
  TEST_VALID(!success);
	for (int i = 0; i < N; i++) {
		printf("0x%08x\t0x%08x\n", base + i * 4, vaddr_read(base + i * 4, 4));
	}
	return 0;

}
static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "[si x] Step x (default 1)",cmd_si},
  { "info","[info r/w] r - reg info , w - watchpoint info",cmd_info},
  { "x", "[x N expr] read N*4 Bytes at expr",cmd_x}

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();//这一步是让用户输入命令并保存到history中
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;//这个args就是除了第一个字符串之后的剩余字符串首指针(不去除首空格)
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {//这里开始比较cmd_table中的预设命令和我们输入的命令了
        if (cmd_table[i].handler(args) < 0) { return; }//如果匹配,就会调用那个命令对应的handler函数
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
