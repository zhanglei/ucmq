#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gtest/gtest.h>

#include "mq_store_file.h"
#include "mq_store_rtag.h"
#include "mq_queue_manage.h"

#include "ucmq_common.h"

using namespace std;

class Unit : public ::testing::Test
{
    virtual void SetUp() {
        log_init_config(&log);
        strcpy(log.log_path, "./log");
        strcpy(log.log_file, "ucmq_test");
        log.log_level = 0;
        log_init(&log);
    }

    virtual void TearDown() {
    }

public:
    log_config_t log;
};

TEST_F(Unit, StoreOperate)
{
    log_debug("begin mq_qm_open_store()...");
    EXPECT_TRUE(mq_qm_open_store());
    log_debug("end mq_qm_open_store()...");
    EXPECT_TRUE(mq_qm_close_store());
}

TEST_F(Unit, TouchDataPath)
{
    EXPECT_FALSE(touch_data_path(NULL));
    EXPECT_FALSE(touch_data_path("/root/anyfile"));   // do not run as root
    EXPECT_FALSE(touch_data_path("/tmp/readonly"));   // -r--r--r--
    EXPECT_FALSE(touch_data_path("/tmp/writeonly"));  // --w--w----
    EXPECT_FALSE(touch_data_path("/tmp/readwrite"));  // -rw-rw-r--
    EXPECT_TRUE(touch_data_path("/tmp/existed"));     // drwxrwx---
    EXPECT_TRUE(touch_data_path("/tmp/nonexisted"));  // the dir not exist before test
    EXPECT_FALSE(touch_data_path("/tmp/level1/level2/level3"));
}

TEST_F(Unit, FindHandleFile)
{
    queue_file_t queue_file;

    strcpy(g_mq_conf.data_file_path, "/usr");  // no permission
    EXPECT_FALSE(find_handle_file(&queue_file, "foo"));

    strcpy(g_mq_conf.data_file_path, "/tmp");
    EXPECT_FALSE(find_handle_file(&queue_file, "queue1"));  // queue name exist, but data file and rtag file not exist
    EXPECT_FALSE(find_handle_file(&queue_file, "queue2"));  // queue name and data file exist, but rtag file not exist
    EXPECT_FALSE(find_handle_file(&queue_file, "queue3"));  // queue name and rtag file exist, but data file not exist
    EXPECT_FALSE(find_handle_file(&queue_file, "queue4"));  // queue name, db file and rtag file exist, but data file does not match rtag file
    EXPECT_FALSE(find_handle_file(&queue_file, "queue5"));  // 2 pairs of db-rtag file

    strcpy(g_mq_conf.data_file_path, "../../data");
    EXPECT_FALSE(find_handle_file(&queue_file, NULL));
    EXPECT_FALSE(find_handle_file(&queue_file, "nonexisted"));

    EXPECT_TRUE(find_handle_file(&queue_file, "foo"));  // queue name, data file and rtag file exist
    EXPECT_STREQ(queue_file.rtag_file, "rtag_000000000001");
    EXPECT_STREQ(queue_file.read_file, "db_000000000001");
    EXPECT_STREQ(queue_file.write_file, "db_000000000001");
    EXPECT_EQ((int) queue_file.rtag_fid, 1);
    EXPECT_EQ((int) queue_file.read_fid, 1);
    EXPECT_EQ((int) queue_file.write_fid, 1);
}

TEST_F(Unit, GetNextReadFile)
{
    char *fname = "/tmp/ucmq_unit_test";
    unlink(fname);

    mq_queue_t mq;
    mq.cur_rdb.opt_count = 123;
    mq.cur_rdb.pos = 321;
    mq.cur_wdb.pos = 456;
    mq.queue_max_size = 1234567;
    mq.queue_delay = 89;
    mq.wlock = 111;
    mq.rtag_fd = mq_sm_rtag_open_next_file(fname);
    EXPECT_TRUE(mq_sm_rtag_write_item(&mq));
    close(mq.rtag_fd);

    char str[1024];
    char buf[1024];
    sprintf(str, "%010u%010u%010u%010u%010u%010u\n",
        mq.cur_rdb.opt_count, mq.cur_rdb.pos, mq.cur_wdb.pos,
        mq.queue_max_size, mq.queue_delay, mq.wlock);
    FILE *fp = fopen(fname, "r");
    fgets(buf, sizeof(buf), fp);
    EXPECT_STREQ(str, buf);
    fclose(fp);
}

TEST_F(Unit, GetNextWriteFile)
{
}

TEST_F(Unit, GetStorageFree)
{
    EXPECT_EQ(get_storage_free("/"), 239);
    EXPECT_EQ(get_storage_free("/home"), 40);
    EXPECT_EQ(get_storage_free("/home1"), 68);
}

TEST_F(Unit, ExtendFileSize)
{
    char *fname = "/tmp/ucmq_unit_test";
    unlink(fname);

    int fd = open(fname, O_RDWR | O_CREAT | O_NONBLOCK, S_IRUSR | S_IWUSR);
    EXPECT_TRUE(extend_file_size(fd, 1024));
    EXPECT_EQ(1024, get_file_size(fname));
    close(fd);

    fd = open(fname, O_RDWR | O_CREAT | O_NONBLOCK, S_IRUSR | S_IWUSR);
    EXPECT_TRUE(extend_file_size(fd, 1024 * 1024));
    EXPECT_EQ(1024 * 1024, get_file_size(fname));
    close(fd);

    fd = open(fname, O_RDWR | O_CREAT | O_NONBLOCK, S_IRUSR | S_IWUSR);
    EXPECT_TRUE(extend_file_size(fd, 1024 * 1024 * 1024));
    EXPECT_EQ(1024 * 1024 * 1024, get_file_size(fname));
    close(fd);
}
