#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
// #include <httpc/httpc.h>
#include <hsf.h>

#define ServerIp    "115.28.179.114"
#define ServerPort	5222
#define ServerName  "ipet.local"
#define HostFjid	"@ipet.local/WSJ"


#ifndef __HFOPENFIRE_H__
#define	__HFOPENFIRE_H__

extern int  SocketFd;
extern int  PingCount;
extern char Base64Des[64];
extern char SmartHostFjid[64];

//喂食机相关变量
extern char CurrentUser[60];
extern char *ActCmd[3];
extern char *Vertline;
extern int fdstate;             //喂食状态,状态0表示可用
extern int wtstate;
extern char CtlWater[4];
extern char CtlFeed[4];
extern hfuart_handle_t Huart;


extern int  USER_FUNC CreateTcpClient(void);
extern void USER_FUNC InitData(void);
extern int  USER_FUNC ConOpenfire(void);
extern void USER_FUNC SendMsg(char*, char*);
extern void USER_FUNC PingOpenfire(void* arg);
extern void USER_FUNC TimerCallback(hftimer_handle_t);
extern void USER_FUNC GetBetweenStrs(char*, char*, char*, char*);
extern long USER_FUNC ConvertToBase64(char *pcOutStr, const char *pccInStr, int iLen);

//喂食机相关接口
extern void USER_FUNC fdthread(void *arg);
extern void USER_FUNC wtthread(void *arg);
extern void USER_FUNC SetTimer(char*);
extern void USER_FUNC CancelTimer(char*);



#endif
