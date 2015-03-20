#include "dllist.h"
#include <stdlib.h>
#include <errno.h>
#include <time.h>

dllist_t *dllistInit(int maxNodes, int condTimeout)
{
    dllist_t *dllist = calloc(1, sizeof(dllist_t));
    dllist->maxNodes = maxNodes;
    pthread_mutex_init(&dllist->mtx, NULL);
    if (maxNodes) {
        dllist->condTimeout = condTimeout;
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
        pthread_cond_init(&dllist->cond, &attr);
        pthread_condattr_destroy(&attr);
    }
    return dllist;
}

void dllistDestroy(dllist_t *dllist)
{
    pthread_mutex_destroy(&dllist->mtx);
    if (dllist->maxNodes) {
        pthread_cond_destroy(&dllist->cond);
    }
    free(dllist);
}

int dllistAppend(dllist_t *dllist, void *data)
{
    struct timespec to;
    clock_gettime(CLOCK_MONOTONIC, &to);
    to.tv_sec += dllist->condTimeout;

    pthread_mutex_lock(&dllist->mtx);

    if (dllist->maxNodes) {
        while (dllist->nodesCount == dllist->maxNodes) {
            if (pthread_cond_timedwait(&dllist->cond, &dllist->mtx, &to) == ETIMEDOUT) {
                pthread_mutex_unlock(&dllist->mtx);
                return 0;
            }
        }
    }

    dllistNode_t *node = calloc(1, sizeof(dllistNode_t));
    node->data = data;
    ((dllistNodeData_t *)data)->node = node;

    if (dllist->tail == NULL) {
        node->prev = NULL;
        node->next = NULL;
        dllist->head = dllist->tail = node;
    } else {
        node->prev = dllist->tail;
        node->next = NULL;
        dllist->tail->next = node;
        dllist->tail = node;
    }
    dllist->nodesCount++;

    pthread_mutex_unlock(&dllist->mtx);

    return 1;
}

void dllistDelete(dllist_t *dllist, void *data)
{
    if (data == NULL) {
        return;
    }

    dllistNode_t *node = ((dllistNodeData_t *)data)->node;
    if (node == NULL) {
        return;
    }

    node->data = NULL;
    ((dllistNodeData_t *)data)->node = NULL;

    pthread_mutex_lock(&dllist->mtx);

    if (node == dllist->head) { // head
        if (dllist->head->next) {
            dllist->head = dllist->head->next;
            dllist->head->prev = NULL;
        } else {
            dllist->head = dllist->tail = NULL;
        }
        node->next = NULL;
    } else if (node == dllist->tail) { // tail
        dllist->tail = dllist->tail->prev;
        dllist->tail->next = NULL;
        node->prev = NULL;
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->prev = NULL;
        node->next = NULL;
    }
    dllist->nodesCount--;

    pthread_mutex_unlock(&dllist->mtx);

    if (dllist->maxNodes) {
        pthread_cond_signal(&dllist->cond);
    }

    free(node);
}

void dllistVisit(dllist_t *dllist, void (*nodeHandler)(void *data))
{
    dllistNode_t *node;

    pthread_mutex_lock(&dllist->mtx);

    node = dllist->head;
    while (node) {
        nodeHandler(node->data);
        node = node->next;
    }

    pthread_mutex_unlock(&dllist->mtx);
}

int dllistCountNodes(dllist_t *dllist)
{
    int count;

    pthread_mutex_lock(&dllist->mtx);
    count = dllist->nodesCount;
    pthread_mutex_unlock(&dllist->mtx);

    return count;
}
