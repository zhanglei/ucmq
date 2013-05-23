#include <iostream>
#include <curl/curl.h>
#include <gtest/gtest.h>

#include "ucmq_common.h"

using namespace std;

class ReadOnly : public ::testing::Test
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

// read only forever
TEST_F(ReadOnly, Forever)
{
    int code;
    char url[1024];

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=readonly",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_READONLY_OK\r\n", buf.data);

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hello,ucmq!",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_READONLY\r\n", buf.data);

    sleep(2);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_READONLY\r\n", buf.data);
}

// read only for 2 seconds
TEST_F(ReadOnly, Soon)
{
    int code;
    char url[1024];

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=readonly&num=2",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_READONLY_OK\r\n", buf.data);

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hello,ucmq!",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_READONLY\r\n", buf.data);

    sleep(2);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
}

// writable
TEST_F(ReadOnly, Writable)
{
    int code;
    char url[1024];

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=readonly",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_READONLY_OK\r\n", buf.data);

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=readonly&num=0",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_READONLY_OK\r\n", buf.data);

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hello,ucmq!",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
}

// reset
TEST_F(ReadOnly, Reset)
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

