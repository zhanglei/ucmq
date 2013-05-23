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
    char *url = (char *) arg;
    buf_t buf;
    CURLcode res;

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
                if (++i >= 3000000) break;
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
    char *url = (char *) arg;
    buf_t buf;
    CURLcode res;
    
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
                EXPECT_STREQ(UCMQ_DATA_116, buf.data);
                if (++i >= 3000000 * MAX_PUT_THREADS / MAX_GET_THREADS) break;
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

class Concurrent : public ::testing::Test
{
    virtual void SetUp() {
        curl_global_init(CURL_GLOBAL_NOTHING);
    }

    virtual void TearDown() {
        curl_global_cleanup();
    }
};

// default value
TEST_F(Concurrent, Start)
{
    int code;

    buf_t buf;
    buf_init(&buf);

    char url[1024];
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status_json",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);

    const char *putpos2 = PUT_POS_STATUS_JSON;
    const char *getpos2 = GET_POS_STATUS_JSON;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    ASSERT_EQ(0, ATOI(buf.data, putpos2));
    ASSERT_EQ(0, ATOI(buf.data, getpos2));

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=maxqueue&num=1000000000",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_MAXQUEUE_OK\r\n", buf.data);

    curl_easy_cleanup(curl);
}

TEST_F(Concurrent, Do)
{
    int i;
    pthread_t put_tid[MAX_PUT_THREADS];
    pthread_t get_tid[MAX_GET_THREADS];
    char put_url[1024];
    char get_url[1024];

    snprintf(put_url, sizeof(put_url), "http://%s:%d/?name=%s&opt=put&data=%s",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME, UCMQ_DATA_116);
    snprintf(get_url, sizeof(get_url), "http://%s:%d/?name=%s&opt=get",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);

    for (i = 0; i < MAX_PUT_THREADS; ++i) {
        pthread_create(&put_tid[i], NULL, do_put, put_url);
    }

    for (i = 0; i < MAX_GET_THREADS; ++i) {
        pthread_create(&get_tid[i], NULL, do_get, get_url);
    }

    for (i = 0; i < MAX_PUT_THREADS; ++i) {
        pthread_join(put_tid[i], NULL);
    }

    for (i = 0; i < MAX_GET_THREADS; ++i) {
        pthread_join(get_tid[i], NULL);
    }
}

/*
// reset, to check the data file and rtag file, do not unremark it
TEST_F(Concurrent, End)
{
    int code;

    buf_t buf;
    buf_init(&buf);

    char url[1024];
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=reset",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_RESET_OK\r\n", buf.data);

    curl_easy_cleanup(curl);
}
*/
