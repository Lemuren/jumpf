#include "fanotify.h"
#include <stdio.h>

int main() {
    jf_event ev;
    jf_fanotify *h = jf_fanotify_init();
    if (!h) {
        perror("Error");
        return 1;
    }

    while (1) {
        int rc = jf_fanotify_next(h, &ev);
        if (rc == 0) {
            printf("%s\n", ev.path);
        } else if (rc < 0) {
            printf("Something went wrong!\n");
        }
    }

    printf("Hello!\n");
	return 0;
}
