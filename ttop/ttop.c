#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <error.h>
#include <pwd.h> //getpwuid;
#include <ncurses.h>
#include <utmp.h>
//클락관련
#include <time.h>
#include <sys/times.h> 
#define INTERVAL 3
enum CpuTag
{
    user = 0, // 초깃값 할당
    NICE,
    SYSTEM,
    idle,
    iowait,
    irq,
    softirq,
    steal,
    quest,
    quest_nice,
};
struct CpuTimeInfo_struct
{
    int user;
    int nice;
    int system;
    int idle;
    int iowait;
    int irq;
    int softirq;
    int steal;
    int quest;
    int quest_nice;
} typedef CpuTimeInfo;
struct TaskInfo_struct
{
    int pid;
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
    char command[1024];
    long utime;
    long stime;
    long startTime;
} typedef TaskInfo;
void getCpuTimeInfo(CpuTimeInfo *cpuTimeInfo);
void getCpuPercentage();
void getUptime();
void getNowTime();
void getUptimeInfo();
void getUserNum();
void getLoadAvgInfo();
void getTaskInfo(TaskInfo **, int *,TaskInfo **);
void getTaskInfoDetail(char *pid, TaskInfo *, TaskInfo *,double,double,int);
void getTaskState(TaskInfo *taskInfoAry, int aryLength);
void getMemory();
void showTaskListInfo(TaskInfo *taskInfoAry, int);
TaskInfo * findTask(int pid, TaskInfo* taskInfoAry,int len);
chtype *transChType(char *str);
int curY = 0; //print할 콘솔의 커서 y 위치
int curX = 0; //print할 콘솔의 커서 x 위치
int keyY =0;
double prevUptime;
void doClsPrt(char *transTempStr, bool isNewLine);
void ttopProcess(CpuTimeInfo *prevCpuTimeInfo, CpuTimeInfo *curCpuTimeInfo,TaskInfo **taskInfoAry,TaskInfo **prevTaskInfoAry);
void sort(TaskInfo* tiAry,int len);
    
int main(int argc, char *argv[])
{
    CpuTimeInfo prevCpuTimeInfo, curCpuTimeInfo;
    TaskInfo *taskInfoAry=NULL;
    TaskInfo *prevTaskInfoAry=NULL;
    memset(&prevCpuTimeInfo, 0, sizeof(CpuTimeInfo));
    initscr();
    cbreak();
    noecho();
    scrollok(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    clock_t start, finish;
    double duration = 0.0;

    start = clock();
    ttopProcess(&prevCpuTimeInfo,&curCpuTimeInfo,&taskInfoAry,&prevTaskInfoAry);
    while (1)
    {
        int key = getch();
        if (key != ERR)
        {
            if ( key == 27 || key == 0)
            {
                //break;
                key = getch();
                
                if (key == 91)
                {                    
                    key = getch();  
                    switch (key)
                    {
                    case 65:
                        keyY--;
                         ttopProcess(&prevCpuTimeInfo,&curCpuTimeInfo,&taskInfoAry,&prevTaskInfoAry);
                         break;
                    case 66:
                        keyY++;
                          ttopProcess(&prevCpuTimeInfo,&curCpuTimeInfo,&taskInfoAry,&prevTaskInfoAry);
                        break;
                    }
                    continue;
                }
            }
            else if (key == 'Q' || key == 'q')
            {
                if(taskInfoAry!=NULL)
                    free(taskInfoAry);
                if(prevTaskInfoAry!=NULL)
                 free(prevTaskInfoAry);
                break;
            }
        }
        /*3초마다 화면 갱신 로직 시작*/
        finish = clock();
        duration = (double)(finish - start) / CLOCKS_PER_SEC;
        if (duration >= 3)
        {
            start = finish;
            ttopProcess(&prevCpuTimeInfo,&curCpuTimeInfo,&taskInfoAry,&prevTaskInfoAry);
        }
        /*3초마다 화면 갱신 로직 끝*/
    }
    endwin();
    return 0;
}

void ttopProcess(CpuTimeInfo *prevCpuTimeInfo,CpuTimeInfo *curCpuTimeInfo,TaskInfo **taskInfoAry,TaskInfo **prevTaskInfoAry)
{
    clear();
    refresh();
    int taskInfoAryLength;
    curY = 0;
    printw("top - ");
   getNowTime();
   getUptimeInfo();
   getUserNum();
   getLoadAvgInfo();
    getTaskInfo(taskInfoAry, &taskInfoAryLength,prevTaskInfoAry);
   sort(*taskInfoAry,taskInfoAryLength);
   getTaskState(*taskInfoAry, taskInfoAryLength);
   getCpuPercentage(prevCpuTimeInfo, curCpuTimeInfo);
   getMemory();
   showTaskListInfo(*taskInfoAry, taskInfoAryLength);
    refresh();
    free(*prevTaskInfoAry);
    *prevTaskInfoAry=*taskInfoAry;    
    *taskInfoAry=NULL;
}

chtype *transChType(char *str)
{
    int strLen = strlen(str) + 1;
    chtype *chtype_ary = (chtype *)malloc(sizeof(chtype) * strLen);
    for (int i = 0; i < strLen; i++)
    {
        chtype_ary[i] = str[i];
    }
    return chtype_ary;
}
void showTaskListInfo(TaskInfo *taskInfoAry, int aryLength)
{
    int sizeY, sizeX;
    getmaxyx(stdscr, sizeY, sizeX);
    char transTempStr[3072];
    sprintf(transTempStr, "%7s %-9s%3s %3s %7s %6s %6s %1s %5s %5s %9s  %-8s", "PID", "USER", "PR", "NI", "VIRT", "RES", "SHR", "S", "%CPU", "%MEM", "TIME+", "COMMAND");
   // curY++;
    doClsPrt(transTempStr, true);
    if(keyY>=aryLength){
        keyY=aryLength-1;
    }else if(keyY<0){
        keyY=0;
    }
    for (int i =keyY; i < aryLength; i++)
    {
        //  if (taskInfoAry[i].pid > 50)
        //      continue;
    int totalSec= taskInfoAry[i].time;
    int min = ((int)totalSec / 6000);
    int sec = ((int)totalSec  % 6000)/100;
    int msec=((int)totalSec  % 6000)%100;
    char transTimeStr[512];
    sprintf(transTimeStr, "%02d:%02d.%02d ", min, sec,msec);

        if (curY > sizeY)
            return;
        char transTempStr[3072];
        char transPr[4];
        if(taskInfoAry[i].pr!=-100)
            sprintf(transPr,"%d",taskInfoAry[i].pr);
        else
            sprintf(transPr,"%s","rt");
        sprintf(transTempStr, "%7d %-9s%3s %3d %7d %6d %6d %c %5.1f %5.1f %10s %-8s ",
                taskInfoAry[i].pid, taskInfoAry[i].user,transPr , taskInfoAry[i].ni, taskInfoAry[i].virt, taskInfoAry[i].res, taskInfoAry[i].shr, taskInfoAry[i].status, taskInfoAry[i].cpu, taskInfoAry[i].mem, transTimeStr, taskInfoAry[i].command);

        doClsPrt(transTempStr, true);
    }
}
void getMemory()
{
    char temp[1024] = "/proc/meminfo";
    char line[1024], tmp[32], size[32];
    FILE *fp = fopen(temp, "r");
    float MemTotal = 0, MemFree = 0, MemAvailable = 0, Buffers, Cached, SwapTotal, SwapFree, SReclaimable;
    while (fgets(line, 1024, fp) != NULL)
    {
        //   printw("%s\n",line);
        if (strstr(line, "MemTotal"))
        {
            sscanf(line, "%s%s", tmp, size);
            MemTotal = atoi(size) / 1024.0;
        }
        else if (strstr(line, "MemFree"))
        {
            sscanf(line, "%s%s", tmp, size);
            MemFree = atoi(size) / 1024.0;
        }
        else if (strstr(line, "MemAvailable"))
        {
            sscanf(line, "%s%s", tmp, size);
            MemAvailable = atoi(size) / 1024.0;
        }
        else if (strstr(line, "Buffers"))
        {
            sscanf(line, "%s%s", tmp, size);
            Buffers = atoi(size);
        }
        else if (strstr(line, "Cached"))
        {
            sscanf(line, "%s%s", tmp, size);
            if (strcmp(tmp, "Cached:") != 0)
            {
                continue;
            }
            Cached = atoi(size);
        }
        else if (strstr(line, "SwapTotal"))
        {
            sscanf(line, "%s%s", tmp, size);
            SwapTotal = atoi(size) / 1024.0;
        }
        else if (strstr(line, "SwapFree"))
        {
            sscanf(line, "%s%s", tmp, size);
            SwapFree = atoi(size) / 1024.0;
        }
        else if (strstr(line, "SReclaimable"))
        {
            sscanf(line, "%s%s", tmp, size);
            SReclaimable = atoi(size);
        }
    }
    float buff_cache = (Buffers + Cached + SReclaimable) / 1024.0;
    float MemUsed = MemTotal - MemFree - buff_cache;
    char transTempStr[3072];
    sprintf(transTempStr, "Mib Mem  : %8.1f total, %8.1f free, %8.1f,used, %8.1f buff/cache", MemTotal, MemFree, MemUsed, buff_cache);
    doClsPrt(transTempStr, true);
    sprintf(transTempStr, "Mib Swap : %8.1f total, %8.1f free, %8.1f,used, %8.1f avail Mem", SwapTotal, SwapFree, SwapTotal - SwapFree, MemAvailable);
      curY++;
    doClsPrt(transTempStr, true);

    fclose(fp);
}
void getUserNum(){
   struct utmp* utmpFp ;
    int userNum=0;
    setutent();
    while ((utmpFp = getutent()) != NULL){
        if (utmpFp->ut_type == USER_PROCESS)
        {
        userNum++;
        }
    }
    endutent();
    char transTempStr[1024];
    sprintf(transTempStr,"%d user, ",userNum);
    doClsPrt(transTempStr, false);
}
void getTaskState(TaskInfo *taskInfoAry, int aryLength)
{
    int Running = 0;
    int Sleeping = 0;
    int Stoped = 0;
    int Zombie = 0;

    for (int i = 0; i < aryLength; i++)
    {
        char stat = taskInfoAry[i].status;
        switch (stat)
        {
        case 'R':
            Running++;
            break;
        case 'S':
            Sleeping++;
            break;
        case 'D':
            Sleeping++;
            break;
        case 'T':
            Stoped++;
            break;
        case 't':
            Stoped++;
            break;
        case 'X':;
            break;
        case 'Z':
            Zombie++;
            break;
        case 'P':;
            break;
        case 'I':
            Sleeping++;
            break;
        }
    }
    char transTempStr[1024];
    sprintf(transTempStr, "Tasks: %d total, %3d running, %3d sleeping, %3d stopped, %3d zombie", aryLength, Running, Sleeping, Stoped, Zombie);
    doClsPrt(transTempStr, true);
}

void getTaskInfoDetail(char *pid, TaskInfo *taskInfo, TaskInfo* prevTaskInfoAry, double curUptime, double prevUptime, int len)
{
    //uid는 effective uid를 사용한다.
    taskInfo->pid = atoi(pid);
    char temp[1024] = "/proc/";
    char procPath[1024], line[2048] , uidStr[512];
    strcat(temp, pid);
    strcpy(procPath, temp);
    strcat(temp, "/status");
    FILE *fp = fopen(temp, "r");
    while (fgets(line, 1024, fp) != NULL)
    {
        if (strstr(line, "Name")){

            sscanf(line, "%s%s ", temp, taskInfo->command);
        }
        else if (strstr(line, "Uid"))
        {

            sscanf(line, "%s%s ", temp, uidStr);
            break;
        }
    }

    fclose(fp);
    uid_t uid = atoi(uidStr);
  
        struct passwd *user_pw = getpwuid(uid);
        char uName[1024];
        strcpy(uName,user_pw->pw_name);
        if (user_pw == NULL)
            return;
        if ( uName== NULL)
            return;
        if(strlen(uName)>=8){
            uName[7]='+';
            uName[8]='\0';
        }
        
        strcpy(taskInfo->user, uName);
    strcpy(temp, procPath);
    strcat(temp, "/stat");

    long pr, ni, cutime, cstime, threadsNum;
    unsigned long utime, stime;
    unsigned long  startTime, totaltime;
    char status;
    char command[1024];
    fp = fopen(temp, "r");
    fscanf(fp, "%*d %s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %ld %ld %ld %ld %ld %*d %lu",
           command, &status, &utime, &stime, &cutime, &cstime, &pr, &ni, &threadsNum, &startTime);
    fclose(fp);
    refresh();
    taskInfo->pr = pr;
    taskInfo->ni = ni;

    taskInfo->status = status;
    taskInfo->utime=utime;
    taskInfo->stime=stime;
    totaltime = utime + stime;
  //  totaltime = totaltime + cstime+cutime; 자식프로세스 사용량까지 포함하는경우
    taskInfo->startTime=startTime;
    TaskInfo * prevTask= findTask( taskInfo->pid, prevTaskInfoAry,len);
    int nFlg=0;
     unsigned long prevTotal;
    if(prevTask==NULL){
     prevTotal=0;
    }else{
        prevTotal = prevTask->utime+prevTask->stime;
    }
   

    double upTimeDiff=curUptime-prevUptime;
    double totalTimeDiff=totaltime-prevTotal;
    long ticks= sysconf (_SC_CLK_TCK);
    double testCpuUsage=100*((totalTimeDiff/ticks)/upTimeDiff);
  
    taskInfo->cpu = (float)((int)((testCpuUsage)*10))*0.1;
    taskInfo->time=utime+stime;
    double memory_usage;

    fp = fopen("/proc/meminfo", "r");
    char  tmp[32], size[32];
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
        if (strstr(line, "VmSize"))
        {

            sscanf(line, "%s%s", tmp, size);
            nVmSize = atoi(size);
        }
        else if (strstr(line, "VmRSS"))
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
    }
    int virt = nVmSize;
    int res = nVmRss;
    int shr = nRssFile + nRssShmem;
    memory_usage = ((res)*100) / total_memory;
    taskInfo->mem = memory_usage;
    taskInfo->virt = virt;
    taskInfo->res = res;
    taskInfo->shr = shr;
    fclose(fp);
}
void getTaskInfo(TaskInfo **taskInfoAry, int *aryLength,TaskInfo **prevTaskInfoAry)
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
    if(*prevTaskInfoAry==NULL){
        *prevTaskInfoAry=(TaskInfo *)malloc(sizeof(TaskInfo) * tot);
        memset(*prevTaskInfoAry,0,sizeof(sizeof(TaskInfo) * tot));
    }
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
                    
                    getTaskInfoDetail(tempFileName, &((*taskInfoAry)[taskIdx]),(*prevTaskInfoAry),curUptime,prevUptime,*aryLength);
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
    prevUptime=curUptime;
}

void getNowTime()
{
    curX = 6;
    int i;
    time_t the_time;
    char buffer[255];
    time(&the_time);

    struct tm *tm_ptr;
    tm_ptr = localtime(&the_time);
    char transTempStr[1024];
    sprintf(transTempStr, "%02d:%02d:%02d up ", tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
    doClsPrt(transTempStr, false);
}

void doClsPrt(char *transTempStr, bool isNewLine)
{
    getch();
    chtype *chtypeStr = transChType(transTempStr);
    waddchstr(stdscr, chtypeStr);
    curX += strlen(transTempStr);
    if (isNewLine)
    {
        curX = 0;
        curY++;
    }
    move(curY, curX);
    free(chtypeStr);
}

void getLoadAvgInfo()
{
    FILE *fp = fopen("/proc/loadavg", "r");
    float first, second, third;
    fscanf(fp, "%f %f %f", &first, &second, &third);
    char transTempStr[1024];
    sprintf(transTempStr, "load average: %01.2f %01.2f %01.2f", first, second, third);
    doClsPrt(transTempStr, true);
    fclose(fp);
}
void getUptimeInfo()
{

    /*업타임 정보 구하기*/
    FILE *fp = fopen("/proc/uptime", "r");
    double totalSec;
    fscanf(fp, "%lf", &totalSec);
    int hour = (int)totalSec / 3600;
    int min = ((int)totalSec % 3600) / 60;
    int sec = ((int)totalSec % 3600) % 60;
    char transTempStr[1024];
    sprintf(transTempStr, "%02d:%02d min, ", hour, min);
    doClsPrt(transTempStr, false);
    fclose(fp);
    /*업타임 정보 구하기 종료*/
    
}

//proc/stat 를 통해 cpu정보 읽기
void getCpuTimeInfo(CpuTimeInfo *cpuTimeInfo)
{
    char s1[10];
    FILE *fp = fopen("/proc/stat", "r");
    fscanf(fp, "%s", s1);
    for (int i = 0; i < 10; i++)
    {
        fscanf(fp, "%d", (int *)cpuTimeInfo + i);
    }
    fclose(fp);
}

void getCpuPercentage(CpuTimeInfo *prevCpuTimeInfo, CpuTimeInfo *curCpuTimeInfo)
{
    getCpuTimeInfo(curCpuTimeInfo);

    float diffTotal = 0;
    int diffValAry[10] = {
        0,
    };

    for (int i = 0; i < 10; i++)
    {
        int prevVal = *((int *)(prevCpuTimeInfo) + i);
        int curVal = *((int *)(curCpuTimeInfo) + i);
        int diffTemp = curVal - prevVal;
        diffValAry[i] = diffTemp;
        diffTotal += diffTemp;
    }
    float cpuPercent[10] = {
        0,
    };
    for (int i = 0; i < 10; i++)
    {
        cpuPercent[i] = diffValAry[i] / diffTotal * 100;
    }
    char transTempStr[1024];
    sprintf(transTempStr, "%%Cpu(s): %4.1f us, %4.1f sy, %4.1f ni, %4.1f id, %4.1f wa, %4.1f hi, %4.1f si, %4.1f st ",
            cpuPercent[user], cpuPercent[SYSTEM], cpuPercent[NICE],
            cpuPercent[idle], cpuPercent[iowait], cpuPercent[irq],
            cpuPercent[softirq], cpuPercent[steal]);
    doClsPrt(transTempStr, true);
    *prevCpuTimeInfo = *curCpuTimeInfo;
}

TaskInfo * findTask(int pid, TaskInfo* taskInfoAry,int len){
    for(int i = 0 ; i < len ; i ++){
        if(taskInfoAry[i].pid==pid){
            return &(taskInfoAry[i]);
        }
    }
    return NULL;
}

void sort(TaskInfo* tiAry,int len){
    int i, j;
    for (i = 0; i < len - 1; i++)
    {
        for (j = i + 1; j < len; j++)
        {
            if(tiAry[i].cpu<tiAry[j].cpu){
            TaskInfo temp =  tiAry[i];
            tiAry[i]=tiAry[j];
            tiAry[j]=temp;
            }else if(tiAry[i].pid>tiAry[j].pid){
                 TaskInfo temp =  tiAry[i];
            tiAry[i]=tiAry[j];
            tiAry[j]=temp;
            }
        }
    }
}