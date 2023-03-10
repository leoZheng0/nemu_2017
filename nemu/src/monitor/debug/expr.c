#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NEQ, TK_NOT, TK_AND, TK_OR, TK_REG, TK_HEX, TK_NUM, TK_MINUS, TK_POINTER

  /* TODO: Add more token types */

};

//根据type获得优先级
int get_priority(int token_type){
  switch (token_type){
    case '+': { // pri = 4
			return 4;
		}
		case '-': { // pri = 4
			return 4;
		}
		case '*': { // pri = 3
			return 3;
		}
		case '/': { // pri = 3
			return 3;
		}
		case TK_NOT: { // pri = 2
			return 2;
		}
		case TK_EQ: { // pri = 5
			return 5;
		}
		case TK_NEQ: { // pri = 5
			return 5;
		}
		case TK_AND: { // pri = 6
			return 6;
		}
		case TK_OR: { // pri = 7
			return 7;
		}
		case TK_MINUS: { // pri = 2
			return 2;
		}
		case TK_POINTER: { // pri = 2
			return 2;
		}
		default: return 0;
  }
}

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus 这个地方首先\\会被c语言转义成一个\,然后变成\+,会被regex转义成正常的+字符
  {"-", '-'},         // minus
  {"\\*", '*'},         // multi
  {"/", '/'},         // divide

  {"==", TK_EQ},         // equal
  {"!=", TK_NEQ},
	{"!", TK_NOT},
	{"&&", TK_AND,},
	{"\\|\\|", TK_OR,},

  {"\\$[a-zA-z]+", TK_REG},		// register
	{"\\b0[xX][0-9a-fA-F]+\\b", TK_HEX}, // hex
	{"\\b[0-9]+\\b", TK_NUM},				// number

  {"\\(", '('},         // left bracket
  {"\\)", ')'},         // right bracket
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )//上面那个rule有多少条目

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 * 就是把rules数组中的各个规则通过regcomp函数“实现”出来，并存到re数组里面
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];//按序识别的token
int nr_token;//记录一共识别了多少个token了
bool bad_expression = false;//代表这个表达式有问题
static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;//承载匹配的字符串

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          
          default: {
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str,substr_start,substr_len);
            strcat(tokens[nr_token].str,"\0");
            
            //这个地方要将大写字母变成小写,方便后面处理
            for (int t = 0; t <= strlen(tokens[nr_token].str); t++) {
							int x = tokens[nr_token].str[t];
							if (x >= 'A' && x <= 'Z') 
                x += ('a' - 'A');
							tokens[nr_token].str[t] = (char)x;
						}

            nr_token++;
          }
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

//验证外括号是否匹配
bool check_parentheses(int p, int q){
  printf("%d\n",tokens[p].type);
  //检测有没有外括号
  if(tokens[p].type!='(' || tokens[q].type!=')'){
    return false;
  }
  else{
    int vir_stack = 0;//虚拟栈大小
    for(int i=p;i<=q;i++){
      if(tokens[i].type=='(') vir_stack++;
      if(tokens[i].type==')') vir_stack--;
      if(vir_stack<0) {
        bad_expression = true;
        return false;
      }
    }

    if(vir_stack!=0){
      bad_expression = true;
      return false;
    }
    else return true;
  }
}

//获得根节点(算符)的位置
int dominant_op(int p, int q) {
  if(bad_expression==true)
    return 0;
  int pos=p,pri=0,cur_pri = 0;//优先级越低越先计算,反之越高越后计算,我们的目标就是找出"最后计算的符号",也就是找到pri最大的那个符号的位置
  int bracket = 0;
  for(int i=p;i<=q;i++){
    //跳过括号内容
    if (tokens[i].type == '(') bracket++;
		if (tokens[i].type == ')') bracket--;
		if (bracket != 0) continue;

    cur_pri = get_priority(tokens[i].type);
    if(cur_pri>=pri){
      pos = i;
      pri = cur_pri;
    }
  }

  //处理--1,!!0这种形式的数据
  if(pri==2){
    for(int i=pos-1;i>=p;i--){
      cur_pri = get_priority(tokens[i].type);
      if(cur_pri==pri){
        pos = i;
      }
    }
  }

  // printf("%d\n",pos);
  return pos;
}


uint32_t eval(int p, int q) {
  if(bad_expression){//错误的表达式就直接退出
    printf("bad expression!");
    assert(0);
  }

  if (p > q) {
    /* Bad expression */
    bad_expression = true;
    return -1;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number（hex，dec，reg）.
     * Return the value of the number.
     */
    int num = 0;
    switch (tokens[p].type){
      case TK_NUM:{
        sscanf(tokens[p].str, "%d", &num);
        return num;
      }
      case TK_HEX:{
        sscanf(tokens[p].str, "%x", &num);
        return num;
      }
      case TK_REG:{
        char* reg_name = (tokens[p].str)+1;
        int reg_length = strlen(reg_name);//看一下是哪个寄存器长度
        //32bit reg
        if(reg_length==3){
          for(int i=R_EAX;i<R_EDI;i++){
            if(strcmp(reg_name,regsl[i])==0){
              return reg_l(i);
            }
          }
          if(strcmp(reg_name,"eip")==0){
            return cpu.eip;
          }
          bad_expression = true;
          return 0;
        }

        //16bit
        else if(reg_length==2){
          for(int i=R_AX;i<R_DI;i++){
            if(strcmp(reg_name,regsw[i])==0){
              return reg_w(i);
            }
          }

          for(int i=R_AL;i<R_BH;i++){
            if(strcmp(reg_name,regsb[i])==0){
              return reg_b(i);
            }
          }

          bad_expression = true;
          return 0;
        }
      }
      default:{
        bad_expression=true;
        return 0;
      }
    }
    bad_expression=true;
    return 0;
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    int op_pos = dominant_op(p,q);
    // val1 = eval(p, op - 1);
    // val2 = eval(op + 1, q);

    // switch (op_type) {
    //   case '+': return val1 + val2;
    //   case '-': /* ... */
    //   case '*': /* ... */
    //   case '/': /* ... */
    //   default: assert(0);
    //}
  }
}
uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    bad_expression = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //这个地方需要识别  负号和减号 , 称号和指针符号
  int bracket = 0;
  for(int i=0;i<nr_token;i++){
    int cur_type = tokens[i].type;
    if(cur_type=='(') bracket++;
    if(cur_type==')') bracket--;
    if(bracket<0){
      *success = false;
      bad_expression = true;
      return 0;
    }

    //形如 a . b(.代表符号),当且仅当a不存在,或a不为任何单一结构(reg,hex,num,右括号)时, .处出现的符号为前缀符号(即minus或pointer)

    if (tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != ')' && 
    tokens[i - 1].type != TK_NUM && tokens[i - 1].type != TK_HEX  && tokens[i - 1].type !=  TK_REG))) {
			tokens[i].type = TK_MINUS;
		}
		if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != ')' && 
    tokens[i - 1].type != TK_NUM && tokens[i - 1].type != TK_HEX  && tokens[i - 1].type !=  TK_REG))) {
			tokens[i].type = TK_POINTER;
		}
  }
  if (bracket != 0) {
		*success = false;
    bad_expression=true;
		return 0;
	}
  uint32_t res = eval(0,nr_token-1);

  if(bad_expression==false){//判断一下是不是bad表达式
      *success = true;
  }
  else{
      *success = false;
  }
  return res;
}
