#include "ucmq_common.h"

extern void buf_init(buf_t *buf)
{
    buf->offset = 0;
    memset(buf->data, 0, sizeof(buf->data));
}

extern size_t curl_read_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
    return fread(ptr, size, nmemb, (FILE *)data);
}

extern size_t curl_write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t len = size * nmemb;
    buf_t *p = (buf_t *) data;
    memcpy(&(p->data[p->offset]), ptr, len);
    p->offset += len;
    p->data[p->offset] = '\0';
    return len;
}

// just a test for receiving large response
extern size_t curl_write_large_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
    do {} while ((uintptr_t) &ptr == 0x01);  // unused
    size_t len = size * nmemb;
    ((buf_t *) data)->offset += len;
    return len;
}

extern void url_encode(const char *src, char *dst, size_t dst_len)
{
    static const char *dont_escape = "._-$,;~()";
    static const char *hex = "0123456789abcdef";
    const char *end = dst + dst_len - 1;

    for (; *src != '\0' && dst < end; src++, dst++) {
        if (isalnum(*(const unsigned char *) src) ||
            strchr(dont_escape, * (const unsigned char *) src) != NULL) {
            *dst = *src;
        } else if (dst + 2 < end) {
            dst[0] = '%';
            dst[1] = hex[(* (const unsigned char *) src) >> 4];
            dst[2] = hex[(* (const unsigned char *) src) & 0xf];
            dst += 2;
        }
    }

    *dst = '\0';
}

