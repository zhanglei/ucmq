#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <curl/curl.h>
#include <gtest/gtest.h>

#include "ucmq_common.h"

using namespace std;

void* do_put(void *arg)
{
    int code;
    char *name = (char *) arg;
    buf_t buf;
    CURLcode res;

    char url[1024];
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=%s%s",
        UCMQ_HOST, UCMQ_PORT, name, UCMQ_DATA_116, name);

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &buf);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    int i = 0;
    while (1) {
        buf_init(&buf);
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
            if (code == HTTP_OK) {
                EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
                if (++i >= MAX_MESSAGES) break;
                continue;
            }
            EXPECT_EQ(HTTP_BAD_REQUEST, code);
            EXPECT_STREQ("HTTPSQS_PUT_FULL\r\n", buf.data);
        }
        sleep(1);
    }
    curl_easy_cleanup(curl);

    return NULL;
}

void* do_get(void *arg)
{
    int code;
    char *name = (char *) arg;
    buf_t buf;
    CURLcode res;
    
    char url[1024];
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=get",
        UCMQ_HOST, UCMQ_PORT, name);

    char data[1024];
    snprintf(data, sizeof(data), "%s%s", UCMQ_DATA_116, name);

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &buf);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    int i = 0;
    while (1) {
        buf_init(&buf);
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
            if (code == HTTP_OK) {
                EXPECT_STREQ(data, buf.data);
                if (++i >= MAX_MESSAGES * MAX_PUT_THREADS / MAX_GET_THREADS) break;
                continue;
            }
            else {
                EXPECT_EQ(HTTP_NOT_FOUND, code);
                EXPECT_STREQ("HTTPSQS_GET_END\r\n", buf.data);
                continue;
            }
        }
        sleep(1);
    }
    curl_easy_cleanup(curl);

    return NULL;
}

class MultiQueue : public ::testing::Test
{
    virtual void SetUp() {
        curl_global_init(CURL_GLOBAL_NOTHING);
    }

    virtual void TearDown() {
        curl_global_cleanup();
    }
};

// create the queues
TEST_F(MultiQueue, Start)
{
    int i;
    int code;
    buf_t buf;
    char name[32];
    char url[1024];
    const char *putpos2 = PUT_POS_STATUS_JSON;
    const char *getpos2 = GET_POS_STATUS_JSON;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &buf);

    for (i = 0; i < MAX_QUEUES; ++i) {
        snprintf(name, sizeof(name), "%s%03d", UCMQ_NAME, i);

        // set maxqueue value in case queue does not exist
        snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=maxqueue&num=3000000",
            UCMQ_HOST, UCMQ_PORT, name);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        buf_init(&buf);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        ASSERT_EQ(HTTP_OK, code);

        // reset the queue
        snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=reset",
            UCMQ_HOST, UCMQ_PORT, name);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        buf_init(&buf);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        ASSERT_EQ(HTTP_OK, code);
        ASSERT_STREQ("HTTPSQS_RESET_OK\r\n", buf.data);

        // check the status
        snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status_json",
            UCMQ_HOST, UCMQ_PORT, name);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        buf_init(&buf);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        ASSERT_EQ(HTTP_OK, code);
        ASSERT_EQ(0, ATOI(buf.data, putpos2));
        ASSERT_EQ(0, ATOI(buf.data, getpos2));
    }
    curl_easy_cleanup(curl);
}

TEST_F(MultiQueue, Do)
{
    int i;
    int j;
    pthread_t put_tid[MAX_QUEUES][MAX_PUT_THREADS];
    pthread_t get_tid[MAX_QUEUES][MAX_GET_THREADS];
    char name[MAX_QUEUES][32];
    int ret;

    for (i = 0; i < MAX_QUEUES; ++i) {
        snprintf(name[i], sizeof(name[i]), "%s%03d", UCMQ_NAME, i);
        for (j = 0; j < MAX_PUT_THREADS; ++j) {
            ret = pthread_create(&put_tid[i][j], NULL, do_put, name[i]);
            ASSERT_EQ(0, ret);
        }
       }

    for (i = 0; i < MAX_QUEUES; ++i) {
        snprintf(name[i], sizeof(name[i]), "%s%03d", UCMQ_NAME, i);
        for (j = 0; j < MAX_GET_THREADS; ++j) {
            ret = pthread_create(&get_tid[i][j], NULL, do_get, name[i]);
            ASSERT_EQ(0, ret);
        }
       }

    for (i = 0; i < MAX_QUEUES; ++i) {
        for (j = 0; j < MAX_PUT_THREADS; ++j) {
            pthread_join(put_tid[i][j], NULL);
        }
        for (j = 0; j < MAX_GET_THREADS; ++j) {
            pthread_join(get_tid[i][j], NULL);
        }
       }
}
