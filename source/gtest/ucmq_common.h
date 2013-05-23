#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// max queues in progress
#define MAX_QUEUES             100
// max messages per queue per thread
#define MAX_MESSAGES           32000
#define MAX_PUT_THREADS        16
#define MAX_GET_THREADS        4

#define UCMQ_HOST              "192.168.3.171"
#define UCMQ_PORT              2013
#define UCMQ_NAME              "foo"
#define UCMQ_DATA_116          "01234567890123456789" \
                               "01234567890123456789" \
                               "01234567890123456789" \
                               "01234567890123456789" \
                               "01234567890123456789" \
                               "0123456789012345"

#define HTTP_OK                200
#define HTTP_BAD_REQUEST       400
#define HTTP_NOT_FOUND         404
#define HTTP_NOT_ALLOWED       405

#define RCOUNT_POS_STATUS      "Read items of current db file: "
#define PUT_POS_STATUS         "Put items of queue : "
#define GET_POS_STATUS         "Get items of queue : "
#define UNREAD_STATUS          "Unread items of queue: "
#define MAX_ITEMS_STATUS       "Maximum number of queue: "
#define DELAY_STATUS           "Delay time of queue: "

#define RCOUNT_STATUS_JSON     "\"rcount\":"
#define PUT_POS_STATUS_JSON    "\"putitems\":"
#define GET_POS_STATUS_JSON    "\"getitems\":"
#define UNREAD_STATUS_JSON     "\"unread\":"
#define MAX_ITEMS_STATUS_JSON  "\"maxqueue\":"
#define DELAY_STATUS_JSON      "\"delay\":"

#define ATOI(txt, key) atoi(strstr(txt, key) + strlen(key))

typedef struct {
    char data[4096];
    size_t offset;
} buf_t;

extern void buf_init(buf_t *buf);
extern size_t curl_read_cb(void *ptr, size_t size, size_t nmemb, void *data);
extern size_t curl_write_cb(void *ptr, size_t size, size_t nmemb, void *data);
extern size_t curl_write_large_cb(void *ptr, size_t size, size_t nmemb, void *data);
extern void url_encode(const char *src, char *dst, size_t dst_len);
