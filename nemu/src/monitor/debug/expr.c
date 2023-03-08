#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NEQ, TK_NOT, TK_AND, TK_OR, TK_REG, TK_HEX, TK_NUM

  /* TODO: Add more token types */

};

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
	{"&&", TK_AND},
	{"\\|\\|", TK_OR},

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
          TK_NOTYPE: break;
          //这个地方要将大写字母变成小写,方便后面处理
          default: {
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str,substr_start,substr_len);
            strcat(tokens[nr_token].str,"\0");
            printf(tokens[nr_token].str);
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

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
