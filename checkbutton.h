
typedef struct btn {
  int pin;
  int state;
  int stime;
  int ptime;
  int _pstart;
  int changed;
} btn_t;

int checkButton(struct btn *b);

