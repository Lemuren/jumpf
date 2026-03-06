#ifndef JF_FANOTIFY_H
#define JF_FANOTIFY_H

#include <time.h>

#define PATH_MAX 4096


typedef struct {
    char path[PATH_MAX];
    time_t atime;
} jf_event;

typedef struct jf_fanotify jf_fanotify;

jf_fanotify *jf_fanotify_init(void);
int jf_fanotify_next(jf_fanotify *h, jf_event *out);
void jf_fanotify_close(jf_fanotify *h);

#endif
