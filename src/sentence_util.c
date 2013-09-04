#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#include "util.h"
#include "sentence_util.h"

#define MAX_LINES 128
#define MAX_LINE_LEN 100


#ifndef XBSERVER
extern char *sentence_file;
#endif

typedef struct sentence {
  char value[MAX_LINE_LEN];
} sentence;

static sentence *sentences;
static sched_t *indices;
static int n_sentences;
static sched_t current_index;


static int
get_newlines(int *new_lines, FILE *f) {
  int ch, l = 0;

  new_lines[0] = l++;

  while (EOF != (ch = getc(f))) {
    if (ch == '\n') {
      new_lines[l++] = ftell(f);
    }
  }
  return l;
}


static void
get_line(char *buf, int pos, FILE *f) {
  fseek(f, pos, SEEK_SET);
  fgets(buf, MAX_LINE_LEN, f);
  if (pos == ftell(f)) {
    buf[0] = '\0';
  }
}


void
init_sentences(char *fname) {
  int new_lines[MAX_LINES];
  current_index = 0;

  // srandom(time(NULL));

  FILE *f = fopen(fname, "r");

  n_sentences = get_newlines(new_lines, f);  

  sentences = xb_malloc(sizeof(sentence) * n_sentences);
  indices = xb_malloc(sizeof(sched_t) * n_sentences);
  init_shuffle(indices, n_sentences);

  int i;
  for (i = 0; i < n_sentences; ++i) {
    get_line(sentences[i].value, new_lines[i], f);    
  }
  fclose(f);
}


void
destroy_sentences() {
  free(sentences);
  sentences = NULL;
  free(indices);
  indices = NULL;
}


char *
get_sentence() {
  #ifndef XBSERVER
  if (sentence_file == NULL) { return NULL; }
  #endif
  /* if we've iterated once already, reshuffle */
  if (current_index == n_sentences) {
    current_index = 0;
    shuffle(indices, n_sentences);
  }
  return sentences[indices[current_index++]].value; 
}
