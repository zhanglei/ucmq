#include <iostream>
#include <curl/curl.h>
#include <gtest/gtest.h>

#include "ucmq_common.h"

class Delay : public ::testing::Test
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
TEST_F(Delay, Start)
{
    int code;
    char url[1024];

    const char *key1 = DELAY_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(0, ATOI(buf.data, key1));

    const char *key2 = DELAY_STATUS_JSON;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status_json",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(0, ATOI(buf.data, key2));
}

// set new value
TEST_F(Delay, NewValue)
{
    int code;
    char url[1024];

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=delay&num=3",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_DELAY_OK\r\n", buf.data);

    const char *key1 = DELAY_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(3, ATOI(buf.data, key1));

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hello,ucmq!",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=get",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_NOT_FOUND, code);
    EXPECT_STREQ("HTTPSQS_GET_END\r\n", buf.data);

    sleep(3);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("hello,ucmq!", buf.data);
}

// lower limit
TEST_F(Delay, LowerLimit)
{
    int code;
    char url[1024];

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=delay&num=2",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_DELAY_OK\r\n", buf.data);
}

// reset
TEST_F(Delay, Reset)
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

