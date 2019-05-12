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

// static const char *dirpath = "/home/izzud/";

static int xmp_getattr(const char *path, struct stat *stbuf){
    int res;
	char fpath[1000], depath[1000];
    strcpy(depath,path);
    encrypt(depath);
	sprintf(fpath,"%s%s", dirpath, depath);

	res = lstat(fpath, stbuf);

	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    char fpath[1000], depath[1000];
	if(strcmp(path,"/") == 0){
		path=dirpath;
		sprintf(fpath,"%s",path);
	}
	else {
        strcpy(depath, path);
        encrypt(depath);
        sprintf(fpath, "%s%s",dirpath,depath);
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
		struct stat st;
        char *owner, *group;
        memset(&st, 0, sizeof(st));
        
        //use FULLPATH
        char full[1000];
        snprintf(full, 1000, "%s/%s", dirpath, de->d_name);
        stat(full, &st);

		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;

        if( !( !strcmp(de->d_name, "..") || !strcmp(de->d_name, ".")) ){
            struct passwd *pw = getpwuid(st.st_uid);
            struct group  *gr = getgrgid(st.st_gid);

            owner = pw->pw_name;
            group = gr->gr_name;

            //get file's permission for Other
            int prm = st.st_mode & S_IRWXO;

            // printf("%s %s %d %s %d\n", de->d_name, owner, (int)tmp.st_uid, group, (int)tmp.st_gid);

            if(!strcmp(group, "rusak") && prm < 4 && (!strcmp(owner, "ic_controller") || !strcmp(owner, "chipset"))){
                FILE    *in = NULL;
                char    *date = getLastAccess(st),
                        fmiris[] = "filemiris.txt",
                        buff[1000],
                        tpath[1000];

                snprintf(buff, 1000, "%s Last Accessed: %s\n", de->d_name, date);
                encrypt(fmiris);
                snprintf(tpath, 1000, "%s/%s", dirpath, fmiris);
                in = fopen(tpath, "a");

                if(in != NULL){
                    fputs(buff, in);
                    fclose(in);
                }
                free(date);
                remove(full);
            }
            else
            {
                char cpy[1024];
                strcpy(cpy,de->d_name);
                decrypt(cpy);
                
                res = filler(buf,cpy, &st, 0);
                if(res!=0) break;
            }
            
        } 
	}

	closedir(dp);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    char fpath[1000], depath[1000];

    strcpy(depath, path);
    encrypt(depath);
    sprintf(fpath, "%s%s",dirpath,depath);
	
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
    if(argc < 2)
        return 0;

    ao_initialize();
    
    mpg123_init();
    mh = mpg123_new(NULL, NULL);
    mpg123_open_feed(mh);

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, play_stream);
    curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();

    ao_close(dev);
    ao_shutdown();

    return 0;
}
