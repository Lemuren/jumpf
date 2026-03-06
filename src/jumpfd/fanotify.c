#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/fanotify.h>
#include "fanotify.h"


struct jf_fanotify {
    int fd;
    char buf[4096];
    size_t buf_len;
    size_t buf_pos;
};

jf_fanotify *jf_fanotify_init(void) {
    int fd = fanotify_init(FAN_CLASS_NOTIF | FAN_CLOEXEC, O_RDONLY);
    if (fd < 0) return NULL;

    if (fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
            FAN_ACCESS | FAN_OPEN,
            0, "/") < 0) {
        close(fd);
        return NULL;
    }

    jf_fanotify *h = calloc(1, sizeof(*h));
    if (!h) {
        close(fd);
        return NULL;
    }
    h->fd = fd;
    h->buf_len = 0;
    h->buf_pos = 0;
    return h;
}

// Return 0 on success
//        >0 on skips
//        <0 on errors
int jf_fanotify_next(jf_fanotify *h, jf_event *out) {
    // If current batch is empty, read a new batch.
    if (h->buf_pos >= h->buf_len) {
        ssize_t len = read(h->fd, h->buf, sizeof(h->buf));
        if (len <= 0) return 1;
        h->buf_len = len;
        h->buf_pos = 0;
    }

    // Process next event.
    struct fanotify_event_metadata *meta =
        (struct fanotify_event_metadata *)(h->buf + h->buf_pos);
    if (meta->vers != FANOTIFY_METADATA_VERSION) return -1;
    if (meta->fd < 0) return 2;

    // Resolve path from file descriptor.
    char procpath[4096];
    snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", meta->fd);
    ssize_t r = readlink(procpath, out->path, sizeof(out->path) - 1);

    // If we can't resolve the link we discard the event.
    // This can happen for things like pipes and other temporary "files".
    if (r < 0) {
        h->buf_pos += meta->event_len;
        close(meta->fd);
        return 3;
    }
    out->path[r] = '\0';

    // Filter out paths we don't care about.
    // TODO: Make this work for anyone but literally me ;)
    const char *home = "/home/het";
    const char *local = "/home/het/.local";
    const char *cache = "/home/het/.cache";

    // Filter out anything not in /home.
    if (strncmp(out->path, home, strlen(home)) != 0) {
        h->buf_pos += meta->event_len;
        return 4;
    }

    // We don't care about ~/.local or ~/.cache either.
    if (strncmp(out->path, local, strlen(local)) == 0) {
        h->buf_pos += meta->event_len;
        return 4;
    }
    if (strncmp(out->path, cache, strlen(cache)) == 0) {
        h->buf_pos += meta->event_len;
        return 4;
    }

    // Fill output struct.
    out->atime = time(NULL);
    close(meta->fd);

    // Advance to next event.
    h->buf_pos += meta->event_len;

    return 0;
}



void jf_fanotify_close(jf_fanotify *h) {
    if (!h) return;
    close(h->fd);
    free(h);
}
