#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <gtest/gtest.h>

#include "mq_queue_manage.h"

#include "ucmq_common.h"

using namespace std;

class SingleFile : public ::testing::Test
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
    char url[1024];
    CURL *curl;
    buf_t buf;
    int putpos;
    int getpos;
};

// default value, both status and status_json
TEST_F(SingleFile, Start)
{
    int code;

    const char *putpos1 = PUT_POS_STATUS;
    const char *getpos1 = GET_POS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    ASSERT_EQ(0, ATOI(buf.data, putpos1));
    ASSERT_EQ(0, ATOI(buf.data, getpos1));

    const char *putpos2 = PUT_POS_STATUS_JSON;
    const char *getpos2 = GET_POS_STATUS_JSON;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status_json",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    ASSERT_EQ(0, ATOI(buf.data, putpos2));
    ASSERT_EQ(0, ATOI(buf.data, getpos2));
}

// try to get message from the empty queue
TEST_F(SingleFile, GetEmpty)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=get",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_NOT_FOUND, code);
    EXPECT_STREQ("HTTPSQS_GET_END\r\n", buf.data);
}

// put message via http get
TEST_F(SingleFile, PutViaHttpGet)
{
    int code;
    //int all_count = mq_qm_get_store_count();

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=put_message_via_http_get",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);

    /*
    int all_count_again = mq_qm_get_store_count();
    EXPECT_EQ(all_count + 1, all_count_again);
    */
}

// check after put, both status and status_json
TEST_F(SingleFile, ChkPutViaHttpGet)
{
    int code;
    //putpos += strlen("put_message_via_http_get") + 12;

    const char *putpos1 = PUT_POS_STATUS;
    const char *getpos1 = GET_POS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(1, ATOI(buf.data, putpos1));
    EXPECT_EQ(0, ATOI(buf.data, getpos1));

    const char *putpos2 = PUT_POS_STATUS_JSON;
    const char *getpos2 = GET_POS_STATUS_JSON;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status_json",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(1, ATOI(buf.data, putpos2));
    EXPECT_EQ(0, ATOI(buf.data, getpos2));
}

// put message via http post
TEST_F(SingleFile, PutViaHttpPost)
{
    int code;
    //int all_count = mq_qm_get_store_count();

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "put message via http post");
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);

    /*
    int all_count_again = mq_qm_get_store_count();
    EXPECT_EQ(all_count + 1, all_count_again);
    */
}

// check after put
TEST_F(SingleFile, ChkPutViaHttpPost)
{
    int code;
    //putpos += strlen("put message via http post") + 12;

    const char *putpos1 = PUT_POS_STATUS;
    const char *getpos1 = GET_POS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(2, ATOI(buf.data, putpos1));
    EXPECT_EQ(0, ATOI(buf.data, getpos1));
}

// get the 1st message
TEST_F(SingleFile, GetFirstMsg)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=get",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("put_message_via_http_get", buf.data);
}

// check after get, both status and status_json
TEST_F(SingleFile, ChkGetFirstMsg)
{
    int code;
    //getpos += strlen("put_message_via_http_get") + 12;

    const char *putpos1 = PUT_POS_STATUS;
    const char *getpos1 = GET_POS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(2, ATOI(buf.data, putpos1));
    EXPECT_EQ(1, ATOI(buf.data, getpos1));

    const char *putpos2 = PUT_POS_STATUS_JSON;
    const char *getpos2 = GET_POS_STATUS_JSON;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status_json",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(2, ATOI(buf.data, putpos2));
    EXPECT_EQ(1, ATOI(buf.data, getpos2));
}

// get the 2nd message
TEST_F(SingleFile, GetSecondMsg)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=get",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("put message via http post", buf.data);
}

// check after get
TEST_F(SingleFile, ChkGetSecondMsg)
{
    int code;
    //getpos += strlen("put message via http post") + 12;

    const char *putpos1 = PUT_POS_STATUS;
    const char *getpos1 = GET_POS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(2, ATOI(buf.data, putpos1));
    EXPECT_EQ(2, ATOI(buf.data, getpos1));
}

// already read
TEST_F(SingleFile, GetNothing)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=get",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_NOT_FOUND, code);
    EXPECT_STREQ("HTTPSQS_GET_END\r\n", buf.data);
}

// put very large message
TEST_F(SingleFile, PutLargeMsg)
{
    int code;
    struct stat finfo;

    int fd = open("file_1M.txt", O_RDONLY);
    ASSERT_GE(fd, 0) << "cannot open file" << endl;
    fstat(fd, &finfo);
    close(fd);

    char expect[] = "Expect:";
    char length[1024];
    snprintf(length, sizeof(length), "Content-Length: %d", (int) finfo.st_size);

    struct curl_slist *headerlist = NULL;
    headerlist = curl_slist_append(headerlist, expect);
    headerlist = curl_slist_append(headerlist, length);

    FILE *fp = fopen("file_1M.txt", "rb");

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_read_cb);
    curl_easy_setopt(curl, CURLOPT_READDATA, fp);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);

    fclose(fp);
}

// check after put
TEST_F(SingleFile, ChkPutLargeMsg)
{
    int code;
    //putpos += 1024 * 1024 + 12;

    const char *putpos1 = PUT_POS_STATUS;
    const char *getpos1 = GET_POS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(3, ATOI(buf.data, putpos1));
    EXPECT_EQ(2, ATOI(buf.data, getpos1));
}

// get the large message
TEST_F(SingleFile, GetLargeMsg)
{
    int code;
    double length;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=get",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_large_cb);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &length);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(1024 * 1024, (int) buf.offset);
    EXPECT_EQ(1024 * 1024, (int) length);
}

// check after get
TEST_F(SingleFile, ChkGetLargeMsg)
{
    int code;
    //getpos += 1024 * 1024 + 12;

    const char *putpos1 = PUT_POS_STATUS;
    const char *getpos1 = GET_POS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(3, ATOI(buf.data, putpos1));
    EXPECT_EQ(3, ATOI(buf.data, getpos1));
}

// put message contains escape character via http post
TEST_F(SingleFile, PutEscMsg)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "\"quoted\"\r\n");
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
}

// check after put
TEST_F(SingleFile, ChkPutEscMsg)
{
    int code;
    //putpos += strlen("\"quoted\"\r\n") + 12;

    const char *putpos1 = PUT_POS_STATUS;
    const char *getpos1 = GET_POS_STATUS;
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=status",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_EQ(4, ATOI(buf.data, putpos1));
    EXPECT_EQ(3, ATOI(buf.data, getpos1));
}

// put an empty message via http get
TEST_F(SingleFile, PutNothingViaHttpGet)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_ERROR\r\n", buf.data);
}

// put an empty message via http post
TEST_F(SingleFile, PutNothingViaHttpPost)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_ERROR\r\n", buf.data);
}

// put message in chinese via http get
TEST_F(SingleFile, PutChnViaHttpGet)
{
    int code;
    char encoded[1024];

    url_encode("UTF8中文字符串", encoded, 1024);
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=%s",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME, encoded);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
}

// put message in chinese via http post
TEST_F(SingleFile, PutChnViaHttpPost)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "UTF8中文字符串");
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);
}

// reset
TEST_F(SingleFile, End)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=reset",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_RESET_OK\r\n", buf.data);
}
