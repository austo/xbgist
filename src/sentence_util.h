#ifndef SENTENCE_UTIL_H
#define SENTENCE_UTIL_H


typedef unsigned short sched_t;

void
init_sentences(char *fname);

void
destroy_sentences();

char *
get_sentence();


#endif