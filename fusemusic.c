#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <mpg123.h>
#include <ao/ao.h>

#define BITS 8

mpg123_handle *mh = NULL;
ao_device *dev = NULL;

static const char *dirpath = "/home/kiki/";

static int xmp_getattr(const char *path, struct stat *stbuf){
    int res;
	char fpath[1000];
	sprintf(fpath,"%s%s", dirpath, depath);

	res = lstat(fpath, stbuf);

	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    char fpath[1000];
	if(strcmp(path,"/") == 0){
		path=dirpath;
		sprintf(fpath,"%s",path);
	}
	else {
        sprintf(fpath, "%s%s",dirpath,path);
    }
    int res = 0;

	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL){
		printf("mama\n");
		if(de->d_type == DT_DIR)
		{
			if(strcmp(de->d_name,"..") != 0 && strcmp(de->d_name,".") != 0)
			{
				char bukafolder[1000];
				sprintf(bukafolder, "%s/%s", path, de->d_name);
				res = panggilan(bukafolder,buf,filler,offset,fi);
			}
		}

		else if(de->d_type !=DT_DIR)
		{
			if(strstr(de->d_name,".mp3"))
			{
				InsertAtTail(fpath,de->d_name);
				struct stat st;
                		memset(&st, 0, sizeof(st));
                		st.st_ino = de->d_ino;
                		st.st_mode = de->d_type << 12;
                		res = (filler(buf, de->d_name, &st, 0));
                        	if(res!=0) break;
			}
		}
}

	closedir(dp);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    char fpath[1000];if(strcmp(path,"/") == 0)
	{
		path=dirpath;
		sprintf(fpath,"%s",path);
	}
	else sprintf(fpath, "%s%s",dirpath,path);
	int res = 0;
  int fd = 0 ;

	(void) fi;
	fd = open(fpath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.readdir	= xmp_readdir,
	.read		= xmp_read,
};

size_t play_stream(void *buffer, size_t size, size_t nmemb, void *userp)
{
    int err;
    off_t frame_offset;
    unsigned char *audio;
    size_t done;
    ao_sample_format format;
    int channels, encoding;
    long rate;

    mpg123_feed(mh, (const unsigned char*) buffer, size * nmemb);
    do {
        err = mpg123_decode_frame(mh, &frame_offset, &audio, &done);
        switch(err) {
            case MPG123_NEW_FORMAT:
                mpg123_getformat(mh, &rate, &channels, &encoding);
                format.bits = mpg123_encsize(encoding) * BITS;
                format.rate = rate;
                format.channels = channels;
                format.byte_format = AO_FMT_NATIVE;
                format.matrix = 0;
                dev = ao_open_live(ao_default_driver_id(), &format, NULL);
                break;
            case MPG123_OK:
                ao_play(dev, audio, done);
                break;
            case MPG123_NEED_MORE:
                break;
            default:
                break;
        }
    } while(done > 0);

    return size * nmemb;
}

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
