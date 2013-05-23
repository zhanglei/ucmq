#include <iostream>
#include <curl/curl.h>
#include <gtest/gtest.h>

#include "ucmq_common.h"

using namespace std;

class MaxQueue : public ::testing::Test
{
    virtual void SetUp() {
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &buf);
    }

    virtual void TearDown() {
        curl_easy_cleanup(curl);
    }

public:
    CURL *curl;
    buf_t buf;
};

// default value, both status and status_json
TEST_F(MaxQueue, Start)
{
    int code;
    char url[1024];

    const char *key1 = MAX_ITEMS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(1000000, ATOI(buf.data, key1));

    const char *key2 = MAX_ITEMS_STATUS_JSON;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status_json",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(1000000, ATOI(buf.data, key2));
}

// set new value
TEST_F(MaxQueue, NewValue)
{
    int code;
    char url[1024];

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=maxqueue&num=35",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_MAXQUEUE_OK\r\n", buf.data);

    const char *key1 = MAX_ITEMS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(35, ATOI(buf.data, key1));

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hello,ucmq",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    for(int i = 0; i < 35; ++i) {
        buf_init(&buf);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        EXPECT_EQ(HTTP_OK, code);
        EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
    }

    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_FULL\r\n", buf.data);
}

// upper limit
TEST_F(MaxQueue, UpperLimit)
{
    int code;
    char url[1024];

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=maxqueue&num=1000000000",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_MAXQUEUE_OK\r\n", buf.data);

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=maxqueue&num=1000000001",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_MAXQUEUE_CANCEL\r\n", buf.data);
}

// lower limit
TEST_F(MaxQueue, LowerLimit)
{
    int code;
    char url[1024];

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=maxqueue&num=0",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_MAXQUEUE_OK\r\n", buf.data);

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=maxqueue&num=-1",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_MAXQUEUE_CANCEL\r\n", buf.data);
}

// reset
TEST_F(MaxQueue, End)
{
    int code;
    char url[1024];

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=reset",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_RESET_OK\r\n", buf.data);
}

