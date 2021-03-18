#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <utmp.h>
#include <dirent.h>
//#include<linux/kdev_t.h>
#include <sys/sysmacros.h>
#include <pwd.h> //getpwuid;
#include <fcntl.h>
#include <stdbool.h>
#include <ncurses.h>
#include <sys/times.h>
struct device
{
    char deviceName[1024];
    long major;
    long minor;
    long tty_nr;
} typedef Device;
struct TaskInfo_struct
{
    int pid;
    char uid[512];
    char user[1024];
    int pr;
    int ni;
    int virt;
    int res;
    int shr;
    int status;
    float cpu;
    float mem;
    int time;
    char command[2048];
    long utime;
    long stime;
    long startTime;
    int vsz;
    int rss;
    char tty[128];
    long tty_nr;
    char stat[128];
    int locked;
    int sid;
    int threadsNum;
    int pgid;
    int fpgid;

} typedef TaskInfo;

void getDeviceInfo(Device **devAry, int *);
int countFileNum(char *dirFullPath);
void optionA(Device *devAry, int devAryLen, TaskInfo *taskInfoAry, int taskInfoAryLength, bool userFlg);
void optionX(Device *devAry, int devAryLen, TaskInfo *taskInfoAry, int taskInfoAryLength, bool userFlg);
void optionANX(Device *devAry, int devAryLen, TaskInfo *taskInfoAry, int taskInfoAryLength, bool userFlg);
void getTaskInfo(TaskInfo **, int *, Device *devAry, int devAryLen);
void getTaskInfoDetail(char *pid, TaskInfo *, double, Device *devAry, int devAryLen);
void getTaskState(TaskInfo *taskInfoAry, int aryLength);
void getMemory();
void prtHeadTitle(bool userFlag);
int sizeY;
int sizeX;
int curY;
int curX;
int main(int argc, char *argv[])
{
    initscr();
    getmaxyx(stdscr, sizeY, sizeX);
    endwin();
    int a = 0, u = 0, x = 0;
    Device *devAry;
    int devAryLen;
    getDeviceInfo(&devAry, &devAryLen);
    TaskInfo *taskInfoAry;
    int taskInfoAryLength;
    getTaskInfo(&taskInfoAry, &taskInfoAryLength, devAry, devAryLen);
    getTaskState(taskInfoAry, taskInfoAryLength);
    if (argc == 1)
    {
        int pid = getpid();
        int targetTTy_nr;

        for (int i = 0; i < taskInfoAryLength; i++)
        {
            if (taskInfoAry[i].pid == pid)
            {
                targetTTy_nr = taskInfoAry[i].tty_nr;
            }
        }
        char transTempStr[1024];
        sprintf(transTempStr, "%7s %-8s %11s %-s", "PID", "TTY", "TIME", "COMMAND");
        transTempStr[sizeX] = '\0';
        printf("%s\n", transTempStr);
        for (int i = 0; i < taskInfoAryLength; i++)
        {
            if (taskInfoAry[i].tty_nr == targetTTy_nr)
            {
                TaskInfo tgTk = taskInfoAry[i];
                unsigned long totalSec = tgTk.time;
                totalSec /= 100.0;
                int hour = (int)totalSec / 3600;
                int min = ((int)totalSec % 3600) / 60;
                int sec = ((int)totalSec % 3600) % 60;
                char transTempStr[2500];
                sprintf(transTempStr, "%7d %-8s    %02d:%02d:%02d %s ", tgTk.pid, tgTk.tty, hour, min, sec, tgTk.command);
                transTempStr[sizeX] = '\0';
                printf("%s\n", transTempStr);
            }
        }
    }
    else
    {
        int optionNum = strlen(argv[1]);
        for (int i = 0; i < optionNum; i++)
        {
            switch (argv[1][i])
            {
            case 'a':
                a++;
                break;
            case 'u':
                u++;
                break;
            case 'x':
                x++;
                break;
                //잘못된 옵션 처리 추가
            }
        }

        if (u != 0)
        {
            if (a != 0 && x != 0)
            {
                optionANX(devAry, devAryLen, taskInfoAry, taskInfoAryLength, true);
            }
            else if (a != 0 && x == 0)
            {
                optionA(devAry, devAryLen, taskInfoAry, taskInfoAryLength, true);
            }
            else if (a == 0 && x != 0)
            {
                optionX(devAry, devAryLen, taskInfoAry, taskInfoAryLength, true);
            }
            else
            {
                optionA(devAry, devAryLen, taskInfoAry, taskInfoAryLength, true);
            }
        }
        else
        {
            if (a != 0 && x != 0)
            {
                optionANX(devAry, devAryLen, taskInfoAry, taskInfoAryLength, false);
            }
            else if (a != 0 && x == 0)
            {
                optionA(devAry, devAryLen, taskInfoAry, taskInfoAryLength, false);
            }
            else if (a == 0 && x != 0)
            {
                optionX(devAry, devAryLen, taskInfoAry, taskInfoAryLength, false);
            }
        }
    }

    free(devAry);
    free(taskInfoAry);
}
void prtHeadTitle(bool userFlag)
{
    char transTempStr[1024];
    if (userFlag)
    {
        sprintf(transTempStr, "%-10s %5s %4s %4s %8s %6s %-7s %-4s %7s %5s %-s", "USER", "PID", "%CPU", "%MEM", "VSZ", "RSS", "TTY", "STAT", "START", "TIME", "COMMAND");
    }
    else
    {
        sprintf(transTempStr, "%10s %-5s %-6s %4s %-s", "PID", "TTY", "STAT", "TIME", "COMMAND");
    }
    transTempStr[sizeX] = '\0';
    printf("%s\n", transTempStr);
}
void prtTaskInfo(bool userFlag, TaskInfo tgTk)
{
    unsigned long totalSec = tgTk.time;
    totalSec /= 100.0;
    int hour = (int)totalSec / 3600;
    int min = ((int)totalSec % 3600) / 60;
    int sec = ((int)totalSec % 3600) % 60;
    char printTTY[128];
    if (strlen(tgTk.tty) == 0)
    {
        strcpy(printTTY, "?");
    }
    else
    {
        strcpy(printTTY, tgTk.tty);
    }
    char *transTempStr;
    transTempStr = (char *)malloc(sizeof(char) * 4500);
    if (userFlag)
    {
        FILE *fp = fopen("/proc/stat", "r");
        char line[1024], tmp[32], size[32];
        long btime = 0;
        while (fgets(line, 1024, fp) != NULL)
        {
            if (strstr(line, "btime"))
            {
                sscanf(line, "%s%s", tmp, size);
                btime = atol(size);
            }
        }
        fclose(fp);
        time_t startTime = tgTk.startTime / 100 + btime;
        struct tm pt = *(localtime(&startTime));
        time_t cur = 0;
        cur = time(NULL);
        struct tm ct = *(localtime(&cur));
        char processStartTime[100] = {
            0,
        };
        if (pt.tm_year == ct.tm_year && pt.tm_mon + 1 == ct.tm_mon + 1 && pt.tm_mday == ct.tm_mday)
        {
            sprintf(processStartTime, "%01d:%02d", pt.tm_hour, pt.tm_min);
        }
        else
        {
            sprintf(processStartTime, "%d월 %d일", pt.tm_mon + 1, pt.tm_mday);
        }

        sprintf(transTempStr, "%-10s %5d  %01.01f  %01.01f %8d %6d %-7s %-6s %5s  %01d:%02d %-s",
                tgTk.user, tgTk.pid, (float)((int)((tgTk.cpu) * 10)) * 0.1, (float)((int)((tgTk.mem) * 10)) * 0.1, tgTk.vsz, tgTk.rss, printTTY,
                tgTk.stat, processStartTime, min, sec, tgTk.command);
    }
    else
    {

        sprintf(transTempStr, "%10d %-5s %-6s %01d:%02d %s ", tgTk.pid, printTTY, tgTk.stat, min, sec, tgTk.command);
    }
    transTempStr[sizeX] = '\0';
    printf("%s\n", transTempStr);
    free(transTempStr);
}
void optionANX(Device *devAry, int devAryLen, TaskInfo *taskInfoAry, int taskInfoAryLength, bool userFlag)
{
    prtHeadTitle(userFlag);
    for (int i = 0; i < taskInfoAryLength; i++)
    {
        TaskInfo tgTk = taskInfoAry[i];
        prtTaskInfo(userFlag, tgTk);
    }
}

void optionX(Device *devAry, int devAryLen, TaskInfo *taskInfoAry, int taskInfoAryLength, bool userFlag)
{
    prtHeadTitle(userFlag);

    char myUid[512];
    TaskInfo myTask;
    int myPid = getpid();
    for (int i = 0; i < taskInfoAryLength; i++)
    {
        if (taskInfoAry[i].pid == myPid)
        {
            myTask = taskInfoAry[i];
            break;
        }
    }
    strcpy(myUid, myTask.uid);
    for (int i = 0; i < taskInfoAryLength; i++)
    {
        if (strcmp(taskInfoAry[i].uid, myUid) == 0)
        {
            TaskInfo tgTk = taskInfoAry[i];
            prtTaskInfo(userFlag, tgTk);
        }
    }
}

void optionA(Device *devAry, int devAryLen, TaskInfo *taskInfoAry, int taskInfoAryLength, bool userFlag)
{
    prtHeadTitle(userFlag);
    for (int i = 0; i < taskInfoAryLength; i++)
    {
        if (taskInfoAry[i].tty_nr != 0)
        {
            TaskInfo tgTk = taskInfoAry[i];
            prtTaskInfo(userFlag, tgTk);
        }
    }
}

void getDeviceInfo(Device **devAry, int *devAryLen)
{

    int tot = 0;
    DIR *dir;
    struct dirent *ent;
    char devPath[1024] = "/dev";
    tot += countFileNum(devPath);
    strcpy(devPath, "/dev/pts");
    tot += countFileNum(devPath);
    *devAry = (Device *)malloc(sizeof(Device) * tot);

    strcpy(devPath, "/dev");
    dir = opendir(devPath);
    int idx = 0;
    if (dir != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            char *tempFileName = ent->d_name;
            int i = 0;
            if (tempFileName[0] == 't' && tempFileName[1] == 't' && tempFileName[2] == 'y')
            {

                char fullPath[2048];
                sprintf(fullPath, "%s/%s", devPath, tempFileName);
                Device *tempDevice = &(*devAry)[idx];

                struct stat statbuf;
                if (lstat(fullPath, &statbuf) == -1)
                {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }
                strcpy(tempDevice->deviceName, tempFileName);
                tempDevice->major = major(statbuf.st_rdev);
                tempDevice->minor = minor(statbuf.st_rdev);
                tempDevice->tty_nr = tempDevice->major * 256 + tempDevice->minor;
                idx++;
            }
        }
        closedir(dir);
    }

    strcpy(devPath, "/dev/pts");
    dir = opendir(devPath);
    if (dir != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            char *tempFileName = ent->d_name;

            char fullPath[2048];
            sprintf(fullPath, "%s/%s", devPath, tempFileName);
            Device *tempDevice = &(*devAry)[idx];

            struct stat statbuf;
            if (lstat(fullPath, &statbuf) == -1)
            {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
            sprintf(tempDevice->deviceName, "pts/%s", tempFileName);
            tempDevice->major = major(statbuf.st_rdev);
            tempDevice->minor = minor(statbuf.st_rdev);
            tempDevice->tty_nr = tempDevice->major * 256 + tempDevice->minor;
            idx++;
        }
        closedir(dir);
    }

    *devAryLen = idx;
}

int countFileNum(char *dirFullPath)
{
    DIR *dir;
    int tot = 0;
    struct dirent *ent;
    dir = opendir(dirFullPath);
    if (dir != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            tot++;
        }
    }
    closedir(dir);
    return tot;
}

void getTaskState(TaskInfo *taskInfoAry, int aryLength)
{

    for (int i = 0; i < aryLength; i++)
    {
        char statStr[128] = {
            0,
        };
        TaskInfo target = taskInfoAry[i];
        char stat = target.status;
        statStr[0] = stat;

        // 1. check "<", "N" : nice to other users
        if (target.ni > 0)
        {
            strcat(statStr, "N");
        }
        else if (target.ni < 0)
        {
            strcat(statStr, "<");
        }

        //2. check "L" : Page Locked;
        if (target.locked != 0)
        {
            strcat(statStr, "L");
        }
        //3.check "s" : Session Leader;
        if (target.pid == target.sid)
        {
            strcat(statStr, "s");
        }
        //4. check "l" : Muili-Threaded
        if (target.threadsNum > 1)
        {
            strcat(statStr, "l");
        }
        if (target.pgid == target.fpgid)
        {
            strcat(statStr, "+");
        }

        strcpy(taskInfoAry[i].stat, statStr);
    }
}

void getTaskInfoDetail(char *pid, TaskInfo *taskInfo, double curUptime, Device *devAry, int devAryLen)
{
    //uid는 effective uid를 사용한다.
    taskInfo->pid = atoi(pid);
    char temp[1024] = "/proc/";
    char procPath[1024], line[2048], uidStr[512];
    strcat(temp, pid);
    strcpy(procPath, temp);
    strcat(temp, "/status");
    FILE *fp = fopen(temp, "r");
    while (fgets(line, 1024, fp) != NULL)
    {
        if (strstr(line, "Uid"))
        {

            sscanf(line, "%s%s ", temp, uidStr);
            break;
        }
    }
    strcpy(taskInfo->uid, uidStr);
    fclose(fp);
    uid_t uid = atoi(uidStr);

    struct passwd *user_pw = getpwuid(uid);
    char uName[1024];
    strcpy(uName, user_pw->pw_name);
    if (user_pw == NULL)
        return;
    if (uName == NULL)
        return;
    if (strlen(uName) >= 8)
    {
        uName[7] = '+';
        uName[8] = '\0';
    }

    strcpy(taskInfo->user, uName);
    strcpy(temp, procPath);
    strcat(temp, "/stat");

    long pr, ni, cutime, cstime;
    int threadsNum;
    unsigned long utime, stime;
    unsigned long startTime, totaltime;
    char status;
    char command[1024];
    int tpDevId, sid, fpgid, pgid;

    fp = fopen(temp, "r");
    fscanf(fp, "%*d %s %c %*d %d %d %d %d %*u %*u %*u %*u %*u %lu %lu %ld %ld %ld %ld %d %*d %lu",
           command, &status, &pgid, &sid, &tpDevId, &fpgid, &utime, &stime, &cutime, &cstime, &pr, &ni, &threadsNum, &startTime);
    fclose(fp);
    //refresh();
    taskInfo->pr = pr;
    taskInfo->ni = ni;
    taskInfo->sid = sid;
    taskInfo->threadsNum = threadsNum;
    taskInfo->pgid = pgid;
    taskInfo->fpgid = fpgid;
    taskInfo->tty_nr = tpDevId;
    // strcpy(taskInfo->command, command);
    taskInfo->status = status;
    taskInfo->utime = utime;
    taskInfo->stime = stime;
    totaltime = utime + stime;
    //totaltime = totaltime + cstime+cutime; //자식프로세스 사용량까지 포함하는경우

    taskInfo->startTime = startTime;
    float seconds = curUptime - (startTime / 100);
    long ticks = sysconf(_SC_CLK_TCK);
    float cpu_usage = ((totaltime / ticks) * 100.0 / seconds);
    taskInfo->cpu = cpu_usage;

    taskInfo->time = utime + stime;
    double memory_usage;
    fp = fopen("/proc/meminfo", "r");
    char tmp[32], size[32];
    fgets(line, sizeof(line), fp);
    double total_memory;
    sscanf(line, "%s%s", tmp, size);
    total_memory = atof(size);
    fclose(fp);

    int nVmSize = 0, nVmRss = 0, nRssFile = 0, nRssShmem = 0;
    strcpy(temp, procPath);
    strcat(temp, "/status");
    fp = fopen(temp, "r");
    while (fgets(line, 1024, fp) != NULL)
    {
        if (strstr(line, "VmSize")) //vsz
        {

            sscanf(line, "%s%s", tmp, size);
            nVmSize = atoi(size);
        }
        else if (strstr(line, "VmRSS")) //rss
        {

            sscanf(line, "%s%s", tmp, size);
            nVmRss = atoi(size);
        }
        else if (strstr(line, "RssFile"))
        {

            sscanf(line, "%s%s", tmp, size);
            nRssFile = atoi(size);
        }
        else if (strstr(line, "RssShmem"))
        {

            sscanf(line, "%s%s", tmp, size);
            nRssShmem = atoi(size);
        }
        else if (strstr(line, "VmLck"))
        {

            sscanf(line, "%s%s", tmp, size);
            taskInfo->locked = atoi(size);
        }
    }
    fclose(fp);
    int virt = nVmSize;
    int res = nVmRss;
    int shr = nRssFile + nRssShmem;
    memory_usage = ((res)*100) / total_memory;
    taskInfo->mem = memory_usage;
    taskInfo->virt = virt;
    taskInfo->res = res;
    taskInfo->shr = shr;
    taskInfo->vsz = nVmSize;
    taskInfo->rss = nVmRss;

    if (tpDevId != 0)
    {
        for (int i = 0; i < devAryLen; i++)
        {
            if (devAry[i].tty_nr == tpDevId)
            {
                strcpy(taskInfo->tty, devAry[i].deviceName);
            }
        }
    }

    const int BUFSIZE = 4096; // should really get PAGESIZE or something instead...
    unsigned char *buffer;
    buffer = (char *)malloc(sizeof(char) * BUFSIZE); // dynamic allocation rather than stack/global would be better

    char cmdPath[1024] = {
        0,
    };
    sprintf(cmdPath, "/proc/%s/cmdline", pid);
    int fd = open(cmdPath, O_RDONLY);
    int nbytesread = read(fd, buffer, BUFSIZE);
    unsigned char *end = buffer + nbytesread;
    for (unsigned char *p = buffer; p < end; /**/)
    {
        strcat(taskInfo->command, p);
        while (*p++)
            ; // skip until start of next 0-terminated section
    }
    if (strlen(taskInfo->command) == 0)
    {
        command[0] = '[';
        command[strlen(command) - 1] = ']';
        strcpy(taskInfo->command, command);
    }

    free(buffer);
    close(fd);
}
void getTaskInfo(TaskInfo **taskInfoAry, int *aryLength, Device *devAry, int devAryLen)
{

    DIR *dir;
    struct dirent *ent;
    dir = opendir("/proc");
    int tot = 0; //전체 프로세스의 개수
    if (dir != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            char *tempFileName = ent->d_name;
            int i = 0;
            while (1)
            {
                if (tempFileName[i] == 0)
                {
                    tot++;
                    break;
                }
                if (tempFileName[i] < '0' || tempFileName[i] > '9')
                    break;
                i++;
            }
        }
        closedir(dir);
    }
    else
    {
        perror("can not get proc dir file list");
    }
    //프로세스 전체갯수 구하기 완료;
    dir = opendir("/proc");
    *taskInfoAry = (TaskInfo *)malloc(sizeof(TaskInfo) * tot); //TaskInfo[tot];

    *aryLength = tot;
    int taskIdx = 0;
    double curUptime;
    FILE *fp = fopen("/proc/uptime", "r");
    fscanf(fp, "%lf", &curUptime);
    fclose(fp);
    if (dir != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            char *tempFileName = ent->d_name;
            int i = 0;
            while (1)
            {
                if (tempFileName[i] == 0)
                {
                    // printw("pid : %s \n", tempFileName);
                    getTaskInfoDetail(tempFileName, &((*taskInfoAry)[taskIdx]), curUptime, devAry, devAryLen);
                    taskIdx++;
                    break;
                }
                if (tempFileName[i] < '0' || tempFileName[i] > '9')
                    break;
                i++;
            }
        }
        closedir(dir);
    }
    else
    {
        perror("can not get proc dir file list");
    }
}
