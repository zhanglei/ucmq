#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <gtest/gtest.h>

#include "ucmq_common.h"

using namespace std;

class MultiFile : public ::testing::Test
{
    virtual void SetUp() {
        snprintf(put_url, sizeof(put_url), "http://%s:%d/?name=%s&opt=put&data=%s",
            UCMQ_HOST, UCMQ_PORT, UCMQ_NAME, UCMQ_DATA_116);
        snprintf(chk_url, sizeof(chk_url), "http://%s:%d/?name=%s&opt=status_json",
            UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
        snprintf(reset_url, sizeof(reset_url), "http://%s:%d/?name=%s&opt=reset",
            UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
        snprintf(get_url, sizeof(get_url), "http://%s:%d/?name=%s&opt=get",
            UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);

        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &buf);
    }

    virtual void TearDown() {
        curl_easy_cleanup(curl);
    }

public:
    char put_url[1024];
    char chk_url[1024];
    char reset_url[1024];
    char get_url[1024];
    CURL *curl;
    buf_t buf;
    int unread;
    int put_cnt;
    int get_cnt;
};

// default value
TEST_F(MultiFile, Start)
{
    int code;

    const char *putpos2 = PUT_POS_STATUS_JSON;
    const char *getpos2 = GET_POS_STATUS_JSON;
    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    ASSERT_EQ(0, ATOI(buf.data, putpos2));
    ASSERT_EQ(0, ATOI(buf.data, getpos2));

    unread = 0;
    put_cnt = 0;
    get_cnt = 0;

    char url[1024];
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=maxqueue&num=1000000000",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_MAXQUEUE_OK\r\n", buf.data);
}

TEST_F(MultiFile, FullFill)
{
    int code;
    int i;
    const char *putpos2 = PUT_POS_STATUS_JSON;
    const char *getpos2 = GET_POS_STATUS_JSON;
    const char *unread2 = UNREAD_STATUS_JSON;
    // put 524286 = (64*1024*1024 - 128) / 128 - 1 messages
    // ((file size - file header) / msg length with msg header)
    // every message's length is 116 = 128 - 12 (total - header)
    int length = strlen(UCMQ_DATA_116) + 12;
    int count = (64*1024*1024 - 128) / length - 1;

    curl_easy_setopt(curl, CURLOPT_URL, put_url);

    for (i = 0; i < count; ++i) {
        buf_init(&buf);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        EXPECT_EQ(HTTP_OK, code);
        EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
        unread++;
        put_cnt++;
    }

    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    //EXPECT_EQ(count * length, ATOI(buf.data, putpos2));
    EXPECT_EQ(put_cnt, ATOI(buf.data, putpos2));
    EXPECT_EQ(get_cnt, ATOI(buf.data, getpos2));
    EXPECT_EQ(unread, ATOI(buf.data, unread2));
    cout << buf.data << endl;

    // put one more message
    curl_easy_setopt(curl, CURLOPT_URL, put_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
    unread++;
    put_cnt++;

    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    //EXPECT_EQ((count + 1) * length, ATOI(buf.data, putpos2));  // the max pos
    EXPECT_EQ(put_cnt, ATOI(buf.data, putpos2));
    EXPECT_EQ(get_cnt, ATOI(buf.data, getpos2));
    EXPECT_EQ(unread, ATOI(buf.data, unread2));
    cout << buf.data << endl;

    // put one more message
    curl_easy_setopt(curl, CURLOPT_URL, put_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
    unread++;
    put_cnt++;

    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    //EXPECT_EQ(length, ATOI(buf.data, putpos2));
    EXPECT_EQ(put_cnt, ATOI(buf.data, putpos2));
    EXPECT_EQ(get_cnt, ATOI(buf.data, getpos2));
    EXPECT_EQ(unread, ATOI(buf.data, unread2));
    cout << buf.data << endl;
}

TEST_F(MultiFile, NearlyFullFill)
{
    int code;
    int i;
    const char *putpos2 = PUT_POS_STATUS_JSON;
    const char *getpos2 = GET_POS_STATUS_JSON;
    const char *unread2 = UNREAD_STATUS_JSON;
    // continue to put 524285 = (64*1024*1024 - 128 - 128) / 128 - 1 messages
    // ((file size - file header - already occupied pos) / msg length with msg header)
    // every message's length is 116 = 128 - 12 (total - header)
    int length = strlen(UCMQ_DATA_116) + 12;
    int count = (64*1024*1024 - 128 - 128) / length - 1;

    curl_easy_setopt(curl, CURLOPT_URL, put_url);

    for (i = 0; i < count; ++i) {
        buf_init(&buf);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        EXPECT_EQ(HTTP_OK, code);
        EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
        unread++;
        put_cnt++;
    }

    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    //EXPECT_EQ((count + 1) * length, ATOI(buf.data, putpos2));
    EXPECT_EQ(put_cnt, ATOI(buf.data, putpos2));
    EXPECT_EQ(get_cnt, ATOI(buf.data, getpos2));
    EXPECT_EQ(unread, ATOI(buf.data, unread2));
    cout << buf.data << endl;

    // put a short message
    char oper_url2[1024];
    char short_msg[] = "S";
    //int short_msg_len = strlen(short_msg) + 12;
    snprintf(oper_url2, sizeof(oper_url2), "http://%s:%d/?name=%s&opt=put&data=%s",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME, short_msg);
    curl_easy_setopt(curl, CURLOPT_URL, oper_url2);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
    unread++;
    put_cnt++;

    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    //EXPECT_EQ((count + 1) * length + short_msg_len, ATOI(buf.data, putpos2));
    EXPECT_EQ(put_cnt, ATOI(buf.data, putpos2));
    EXPECT_EQ(get_cnt, ATOI(buf.data, getpos2));
    EXPECT_EQ(unread, ATOI(buf.data, unread2));
    cout << buf.data << endl;

    // cannot fill a 128 bytes msg in the data file
    // put one more message, it will be stored in a new data file
    curl_easy_setopt(curl, CURLOPT_URL, put_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
    unread++;
    put_cnt++;

    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    //EXPECT_EQ(length, ATOI(buf.data, putpos2));
    EXPECT_EQ(put_cnt, ATOI(buf.data, putpos2));
    EXPECT_EQ(get_cnt, ATOI(buf.data, getpos2));
    EXPECT_EQ(unread, ATOI(buf.data, unread2));
    cout << buf.data << endl;
}

TEST_F(MultiFile, ReadAll)
{
    int code;
    int i;
    //const char *rcount2 = RCOUNT_STATUS_JSON;
    const char *putpos2 = PUT_POS_STATUS_JSON;
    const char *getpos2 = GET_POS_STATUS_JSON;
    const char *unread2 = UNREAD_STATUS_JSON;

    // read 524287 = (64*1024*1024 - 128) / 128 messages
    // ((file size - file header) / msg length with msg header)
    // every message's length is 116 = 128 - 12 (total - header)
    int length = strlen(UCMQ_DATA_116) + 12;
    int count = (64*1024*1024 - 128) / length;

    curl_easy_setopt(curl, CURLOPT_URL, get_url);

    // read all of the msg in the 1st data file
    for (i = 0; i < count; ++i) {
        buf_init(&buf);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        EXPECT_EQ(HTTP_OK, code);
        EXPECT_STREQ(UCMQ_DATA_116, buf.data);
        unread--;
        get_cnt++;
    }

    // check
    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    //EXPECT_EQ(length, ATOI(buf.data, putpos2));
    EXPECT_EQ(put_cnt, ATOI(buf.data, putpos2));
    //EXPECT_EQ(count * length, ATOI(buf.data, getpos2));
    EXPECT_EQ(get_cnt, ATOI(buf.data, getpos2));
    EXPECT_EQ(unread, ATOI(buf.data, unread2));
    //EXPECT_EQ(count, ATOI(buf.data, rcount2));
    cout << buf.data << endl;

    // read all of the msg in the 2nd data file
    curl_easy_setopt(curl, CURLOPT_URL, get_url);
    for (i = 0; i < count - 1; ++i) {
        buf_init(&buf);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        EXPECT_EQ(HTTP_OK, code);
        EXPECT_STREQ(UCMQ_DATA_116, buf.data);
        unread--;
        get_cnt++;
    }

    // read the short message
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("S", buf.data);
    unread--;
    get_cnt++;

    // check again
    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    //EXPECT_EQ(length, ATOI(buf.data, putpos2));
    EXPECT_EQ(put_cnt, ATOI(buf.data, putpos2));
    //int short_msg_len = strlen("S") + 12;
    //EXPECT_EQ((count-1) * length + short_msg_len, ATOI(buf.data, getpos2));
    EXPECT_EQ(get_cnt, ATOI(buf.data, getpos2));
    EXPECT_EQ(unread, ATOI(buf.data, unread2));
    //EXPECT_EQ(count, ATOI(buf.data, rcount2));
    cout << buf.data << endl;

    // read the only one msg in the 3rd data file
    curl_easy_setopt(curl, CURLOPT_URL, get_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ(UCMQ_DATA_116, buf.data);
    unread--;
    get_cnt++;

    // check again
    curl_easy_setopt(curl, CURLOPT_URL, chk_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    //EXPECT_EQ(length, ATOI(buf.data, putpos2));
    EXPECT_EQ(put_cnt, ATOI(buf.data, putpos2));
    //EXPECT_EQ(length, ATOI(buf.data, getpos2));
    EXPECT_EQ(get_cnt, ATOI(buf.data, getpos2));
    EXPECT_EQ(0, ATOI(buf.data, unread2));
    //EXPECT_EQ(1, ATOI(buf.data, rcount2));
    cout << buf.data << endl;
}

// reset, to check the data file and rtag file, do not unremark it
TEST_F(MultiFile, End)
{
    int code;

    curl_easy_setopt(curl, CURLOPT_URL, reset_url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_RESET_OK\r\n", buf.data);
}
