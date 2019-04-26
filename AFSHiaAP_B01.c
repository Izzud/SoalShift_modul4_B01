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
#define key 17

static const char *dirpath = "/home/izzud/shift4";

char pool[]="qE1~ YMUR2\"`hNIdPzi%^t@(Ao:=CQ,nx4S[7mHFye#aT6+v)DfKL$r?bkOGB>}!9_wV']jcp5JZ&Xl|\\8s;g<{3.u*W-0qE1~ YMUR2\"`hNIdPzi%^t@(Ao:=CQ,nx4S[7mHFye#aT6+v)DfKL$r?bkOGB>}!9_wV']jcp5JZ&Xl|\\8s;g<{3.u*W-0";

char* getTimeStamp(){
    time_t raw;

    struct tm *timeinfo;
    /*dynamic array */
    char *re = (char *)calloc(20, sizeof(char));

    time(&raw);                         //get "raw date"
    timeinfo = localtime(&raw);         //get info from "raw date" in local timezone

    strftime(re, 20, "%Y-%m-%d-%R:%S", timeinfo);

    return re;
}

char* getLastAccess(struct stat sb){
    time_t raw = sb.st_atime;

    struct tm *timeinfo;
    /*dynamic array */
    char *re = (char *)calloc(20, sizeof(char));

    timeinfo = localtime(&raw);         //get info from "raw date" in local timezone

    strftime(re, 20, "%d/%m/%Y-%R:%S", timeinfo);

    return re;
}

void encrypt(char temp[]){
    int aa = 0,
        size = strlen(temp);
    
    for (aa = 0; aa < size; aa++){
        int index;
        char *e, ch = temp[aa];
        
        if(ch != '/'){
            int shift = key%92;    
            e = strchr(pool, ch);
            index = (int)(e - pool);
            ch = pool[index+shift];
        }
        temp[aa] = ch;
    }
}

void decrypt(char temp[]){
    int aa = 0,
        size = strlen(temp);

    for (aa = 0; aa < size; aa++){
        int idx;
        char *e, ch = temp[aa];

        if(ch != '/'){
            int shift = key%92;      
            e = strrchr(pool, ch);
            idx = (int)(e - pool);
            ch = pool[idx-shift];
        }
        temp[aa] = ch;
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf){
    int res;
	char fpath[1000], depath[1000];
    strcpy(depath,path);
    encrypt(depath);
	sprintf(fpath,"%s%s", dirpath, depath);
	
	// puts(dirpath);
	// puts(path);
	//puts(fpath);

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

static int xmp_mkdir(const char *path, mode_t mode)
{
    int res;

    char fpath[1000], depath[1000];
    if(strstr(path, "/YOUTUBER/") != NULL)
        mode = 0750;

    strcpy(depath, path);
    encrypt(depath);
    sprintf(fpath, "%s%s",dirpath,depath);
    
    res = mkdir(fpath, mode);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int res, ytb = 0;

    char fpath[1000], depath[1200], suffix[5], buff[1200];

    strcpy(depath, path);
    if(strstr(path, "/YOUTUBER/") != NULL){
        ytb = 1;
        mode = 0640;
        char *ext = strrchr(path,'.');
        if(ext && strcmp(ext, ".swp") != 0){
            strcpy(suffix,".iz1");   
            encrypt(suffix);
        }
    }
    encrypt(depath);
    sprintf(fpath, "%s%s",dirpath,depath);

    res = mknod(fpath, mode, rdev);
    if(res == -1)
        return -errno;

    if(ytb){
        pid_t child_id;
        child_id = fork();

        if(child_id == 0){
            snprintf(buff, 1200, "%s%s", fpath, suffix);
            char *argv[] = {"mv", fpath, buff, NULL};
            execv("/bin/mv", argv);
        }
    }
    return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
    int res = 0;
    char fpath[1000], depath[1000], *ext = strrchr(path, '.');

    if(ext && !strcmp(ext, ".iz1")){
        pid_t child_id;
        child_id = fork();

        if(child_id == 0){
            char *argv[] = {"zenity", "--error", "--text=File ekstensi iz1 tidak boleh diubah permissionnya.\n", NULL};
            execv("/usr/bin/zenity", argv);
        }

        //system call
        //system("zenity --error --text=\"File ekstensi iz1 tidak boleh diubah permissionnya.\n\"");
        //puts(fpath);
        return 0;
    }

    strcpy(depath, path);
    encrypt(depath);
    sprintf(fpath, "%s%s",dirpath,depath);

    // puts(fpath);

    res = chmod(fpath, mode);
    if(res == -1)
        return -errno;
    
    return 0;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int fd,
        res;
	char fpath[1000],
        tmp1[1000],
        tmp2[1000],
        depath[1000], 
        backup[1200], 
        buff[1200], 
        directory[1000];

    strcpy(tmp1, path);
    strcpy(tmp2, path);

    strcpy(directory, dirname(tmp1));
    if(directory[strlen(directory)-1] != '/')
        strcat(directory, "/");

    strcat(directory, "Backup");

    encrypt(directory);
    snprintf(backup, 1200, "%s%s", dirpath, directory);

    mkdir(backup, 0744);
    
    char ext[12];
    strcpy(ext, strrchr(path,'.'));
    encrypt(ext);

    memset(depath, 0, 1000);
    strcpy(depath, path);

    encrypt(depath);
    sprintf(fpath, "%s%s",dirpath,depath);
    
	fd = open(fpath, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);

    pid_t child_id;
    child_id = fork();

    if(child_id == 0){

        int len = strlen(depath);
        char timeStamp[25],
            fName[1200],
            fbase[1200];

        strcpy(fName, basename(tmp2));

        printf("fName: %s\n", fName);
        strncpy(fbase, fName, strlen(fName) - strlen(ext));
        encrypt(fbase);

        strcpy(timeStamp, "_");
        strcat(timeStamp, getTimeStamp());
        encrypt(timeStamp);
        
        strncpy(fName, depath, len-4);

        snprintf(buff, 1200, "%s/%s%s%s", backup, fbase, timeStamp, ext);
        // puts(buff);

        char *argv[] = {"cp", fpath, buff, NULL};
        execv("/bin/cp", argv);
    }
    
	return res;
}

static int xmp_truncate(const char *path, off_t size)
{
    int res;
	char fpath[1000],depath[1000];

    strcpy(depath, path);
    encrypt(depath);
    sprintf(fpath, "%s%s",dirpath,depath);

    res = truncate(fpath, size);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
    int res;
	char fpath[1000],depath[1000];

    strcpy(depath, path);
    encrypt(depath);
    sprintf(fpath, "%s%s",dirpath,depath);

    /* don't use utime/utimes since they follow symlinks */
    res = utimensat(0, fpath, ts, AT_SYMLINK_NOFOLLOW);
    if (res == -1)
            return -errno;
    return 0;
}

static int xmp_unlink(const char *path)
{

    int res = 0;
	char fpath[1000],
        tmp1[1000],
        tmp2[1000],
        depath[1000],
        buff[1200], 
        recycle[1200],
        directory[1000];

    strcpy(tmp1, path);
    strcpy(tmp2, path);

    strcpy(directory, dirname(tmp1));
    if(directory[strlen(directory)-1] != '/')
        strcat(directory, "/");

    strcat(directory, "RecycleBin");

    encrypt(directory);
    snprintf(recycle, 1200, "%s%s", dirpath, directory);

    mkdir(recycle, 0744);

    char ext[12];
    strcpy(ext, strrchr(path,'.'));
    encrypt(ext);

    char fName[1200],       //file name
        temp[1200],
        fbase[1200];        //fName w/o ext

    strcpy(fName, basename(tmp2));

    memset(fbase, 0, 1200);
    strncpy(fbase, fName, strlen(fName) - strlen(ext));
    // printf("%d %d %s\n", (int)strlen(fName), (int)strlen(ext), fbase);

    snprintf(temp, 1200, "/%s_deleted_%s",fbase,getTimeStamp());
    encrypt(temp);
    snprintf(buff, 1200, "%s%s", recycle, temp);

    mkdir(buff, 0744);

    strcat(buff,"/");
    strcat(buff, fName);

    //get file path
    strcpy(depath, path);
    encrypt(depath);
    sprintf(fpath, "%s%s",dirpath,depath);

    // rename(fpath, buff);

    pid_t child_id;
    child_id = fork();

    if(child_id == 0){

        // char fbs[1200];
        // memset(fbs, 0, 1200);
        // strncpy(fbs, fpath, strlen(fpath) - strlen(ext));
        // //encrypt(fbs);
        // strcat(fbs, "*");s

        char *argv[] = {"cp", fpath, buff, NULL};

        decrypt(fpath);
        decrypt(buff);
        puts(fpath);
        puts(buff);
        execv("/bin/cp", argv);
    }

    res = unlink(fpath);
    if(res == -1)
        return -errno;

    return 0;
}

static int xmp_rename(const char *from, const char *to)
{
    int res;

    res = rename(from, to);
    if(res == -1)
        return -errno;

    return 0;
}

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.readdir	= xmp_readdir,
	.read		= xmp_read,
    .mkdir      = xmp_mkdir,
    .mknod      = xmp_mknod,
    .chmod	    = xmp_chmod,
    .write      = xmp_write,
    .truncate   = xmp_truncate,
    .utimens    = xmp_utimens,
    .unlink	    = xmp_unlink,
    .rename     = xmp_rename,
};

int main(int argc, char *argv[]){
    // char f[]="que?";
    // char *q = f;
    // strcat(q, ".como estas");
    // printf("%s\n%s\n", f, q);
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
