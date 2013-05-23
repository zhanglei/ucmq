#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <gtest/gtest.h>

#include "ucmq_common.h"

using namespace std;

class UrlException : public ::testing::Test
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
};

TEST_F(UrlException, NoParam)
{
    int code;

    snprintf(url, sizeof(url), "http://%s:%d/", UCMQ_HOST, UCMQ_PORT);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_NOT_ALLOWED, code);
}

TEST_F(UrlException, InvalidOpt)
{
    int code;

    // no opt
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s", UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_NOT_ALLOWED, code);

    // opt not recognized
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=uptou",
        UCMQ_HOST, UCMQ_PORT, UCMQ_NAME);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_NOT_ALLOWED, code);
}

TEST_F(UrlException, NameTooLong)
{
    int code;

    // name length up to 32
    char *name = "Very_Very_Long_56789012345678901";
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hi",
        UCMQ_HOST, UCMQ_PORT, name);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_OK, code);
    EXPECT_STREQ("HTTPSQS_PUT_OK\r\n", buf.data);

    // exceed 32
    name = "Very_Very_Long_567890123456789012";
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hi",
        UCMQ_HOST, UCMQ_PORT, name);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_ERROR\r\n", buf.data);
}

TEST_F(UrlException, InvalidName)
{
    int code;

    char *name = ".invalid";
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hi",
        UCMQ_HOST, UCMQ_PORT, name);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_ERROR\r\n", buf.data);

    name = "消息队列";
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hi",
        UCMQ_HOST, UCMQ_PORT, name);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_UNPARSE_URI\r\n", buf.data);

    char str[1024];
    url_encode("消息队列", str, sizeof(str));
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hi",
        UCMQ_HOST, UCMQ_PORT, str);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_ERROR\r\n", buf.data);

    name = "message%20queue";
    snprintf(url, sizeof(url), "http://%s:%d/?name=%s&opt=put&data=hi",
        UCMQ_HOST, UCMQ_PORT, name);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    buf_init(&buf);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_ERROR\r\n", buf.data);
}

TEST_F(UrlException, FillDb)
{
    int code;
    struct stat finfo;

    int fd = open("file_fill_db.txt", O_RDONLY);
    ASSERT_GE(fd, 0) << "cannot open file" << endl;
    fstat(fd, &finfo);
    close(fd);

    char expect[] = "Expect:";
    char length[1024];
    snprintf(length, sizeof(length), "Content-Length: %d", (int) finfo.st_size);

    struct curl_slist *headerlist = NULL;
    headerlist = curl_slist_append(headerlist, expect);
    headerlist = curl_slist_append(headerlist, length);

    FILE *fp = fopen("file_fill_db.txt", "rb");

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

TEST_F(UrlException, ExceedDb)
{
    int code;
    struct stat finfo;

    int fd = open("file_exceed_db.txt", O_RDONLY);
    ASSERT_GE(fd, 0) << "cannot open file" << endl;
    fstat(fd, &finfo);
    close(fd);

    char expect[] = "Expect:";
    char length[1024];
    snprintf(length, sizeof(length), "Content-Length: %d", (int) finfo.st_size);

    struct curl_slist *headerlist = NULL;
    headerlist = curl_slist_append(headerlist, expect);
    headerlist = curl_slist_append(headerlist, length);

    FILE *fp = fopen("file_exceed_db.txt", "rb");

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
    EXPECT_EQ(HTTP_BAD_REQUEST, code);
    EXPECT_STREQ("HTTPSQS_PUT_ERROR\r\n", buf.data);

    fclose(fp);
}
