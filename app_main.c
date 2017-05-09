#include "hfopenfire.h"
#include "../example.h"

#if (EXAMPLE_USE_DEMO==USER_SOCKET_DEMO)

static int module_type= HFM_TYPE_LPB100;
const int hf_gpio_fid_to_pid_map_table[HFM_MAX_FUNC_CODE]=
{
	HF_M_PIN(2),	//HFGPIO_F_JTAG_TCK
	HFM_NOPIN,	//HFGPIO_F_JTAG_TDO
	HFM_NOPIN,	//HFGPIO_F_JTAG_TDI
	HF_M_PIN(5),	//HFGPIO_F_JTAG_TMS
	HFM_NOPIN,		//HFGPIO_F_USBDP
	HFM_NOPIN,		//HFGPIO_F_USBDM
	HF_M_PIN(39),	//HFGPIO_F_UART0_TX
	HF_M_PIN(40),	//HFGPIO_F_UART0_RTS
	HF_M_PIN(41),	//HFGPIO_F_UART0_RX
	HF_M_PIN(42),	//HFGPIO_F_UART0_CTS
	
	HF_M_PIN(27),	//HFGPIO_F_SPI_MISO
	HF_M_PIN(28),	//HFGPIO_F_SPI_CLK
	HF_M_PIN(29),	//HFGPIO_F_SPI_CS
	HF_M_PIN(30),	//HFGPIO_F_SPI_MOSI
	
	HF_M_PIN(23),//HFM_NOPIN,	//HFGPIO_F_UART1_TX,
	HFM_NOPIN,	//HFGPIO_F_UART1_RTS,
	HF_M_PIN(8),	//HFGPIO_F_UART1_RX,
	HFM_NOPIN,	//HFGPIO_F_UART1_CTS,
	
	HF_M_PIN(43),	//HFGPIO_F_NLINK
	HF_M_PIN(44),	//HFGPIO_F_NREADY
	HF_M_PIN(45),	//HFGPIO_F_NRELOAD
	HF_M_PIN(7),	//HFGPIO_F_SLEEP_RQ
	HFM_NOPIN,//HF_M_PIN(8),	//HFGPIO_F_SLEEP_ON
		
	HF_M_PIN(15),		//HFGPIO_F_WPS
	HFM_NOPIN,		//HFGPIO_F_RESERVE1
	HFM_NOPIN,		//HFGPIO_F_RESERVE2
	HFM_NOPIN,		//HFGPIO_F_RESERVE3
	HFM_NOPIN,		//HFGPIO_F_RESERVE4
	HFM_NOPIN,		//HFGPIO_F_RESERVE5
	
	HFM_NOPIN,	//HFGPIO_F_USER_DEFINE
};

const hfat_cmd_t user_define_at_cmds_table[]=
{
	{NULL,NULL,NULL,NULL} //the last item must be null
};


void app_init(void)
{
	u_printf("app_init\n");
}

/***************************************************
FUNC: 测试回调监听串口与数据存储接口，测试Git备份
NAME: app_main.c
TIME: 2017-4-24 09:38:19
***************************************************/
void USER_FUNC RecvMsg(void*);
void USER_FUNC CheckPingCount(void*);
void USER_FUNC LogicHandle(char*);
void USER_FUNC TimedFeed(void *arg);
void USER_FUNC correctTime(void* arg);
void USER_FUNC initNTP(void);
int  USER_FUNC DataStorage(int, int);
int  USER_FUNC GetUser(void);
void USER_FUNC HttpInit(void);
int  USER_FUNC BCDToInt(char bcd);
int  USER_FUNC create_udp(void);
static int USER_FUNC uart_recv_callback(uint32_t, char*, uint32_t, uint32_t);

///定义测试变量
int udp_fd;
struct sockaddr_in sevraddr;


///定义变量
int  SocketFd;
int  MaxFd;
int  PingCount;
char Base64Des[64];
char SmartHostFjid[64];
hftimer_handle_t PingTimer = NULL;
char CurrentUser[60];

//喂食喂水变量
char *Vertline = "|";
char *ActCmd[3];
int fdstate = 0;             //喂食状态,状态0表示可用
int wtstate = 0;
char CtlWater[4] = {0xFF,0x02,0x01,0x00};
char CtlFeed[4]  = {0xFF,0x03,0x01,0x00};
hfuart_handle_t Huart;

//定时变量,接受更新
static char RealTime[6];
static int Clock_H;
static int Clock_M;

//HTTP请求变量
char mac[16];
char userName[30];		//用户名
char httpURL[32] = "http://115.28.179.114:8885";


//WSJ逻辑处理
void USER_FUNC LogicHandle(char *Content)
{

	u_printf("Content is %s\n", Content);
	char *pStr = strtok(Content, "#");
	int nInt = 0;
	int jInt;
	char *CtlCmd[3];
	char Readbuf[160];
	char Cmdbuf[30];

	while(pStr)
	{
		CtlCmd[nInt] = pStr;
		nInt++;
		pStr = strtok(NULL, "#");
	}

	//控制喂水、喂食，指令格式：action#1,10
	if (strncmp(CtlCmd[0], "action", sizeof("action")) == 0)
	{
		char *qStr = strtok(CtlCmd[1], ",");
		jInt = 0;
		while(qStr)
		{
			ActCmd[jInt] = qStr;
			jInt++;
			qStr = strtok(NULL, ",");
		}
		u_printf("ActCmd[0] is :%s\n", ActCmd[0]);
		
		if (strncmp(ActCmd[0],"1",sizeof("1"))==0)
		{
			if (fdstate == 0)						//如果当前没有喂食操作
			{
				fdstate = 1;                        //将喂食状态改为1，触发喂食线程
			}
			else
			{
				SendMsg(CurrentUser, "fdbusy");
				u_printf("Warning: fdbusy!\n");
			}
		}
		else if (strncmp(ActCmd[0], "0", sizeof("0"))==0)
		{
			if (wtstate == 0)
			{
				wtstate = 1;                         //将喂水状态改为1，触发喂水线程
			}
			else
			{
				SendMsg(CurrentUser, "wtbusy");
				u_printf("Warning: wtbusy!\n");
			}
		}
	}
	else if(strncmp(CtlCmd[0], "set", sizeof("set")) == 0)					//定时设置(添加和修改)
	{
		memset(Cmdbuf, 0, sizeof(Cmdbuf));
		strcpy(Cmdbuf, CtlCmd[1]);
		SetTimer(Cmdbuf);
	}
	else if(strncmp(CtlCmd[0], "cancel", sizeof("cancel")) == 0)			//取消定时设置
	{
		memset(Cmdbuf, 0, sizeof(Cmdbuf));
		strcpy(Cmdbuf, CtlCmd[1]);
		CancelTimer(Cmdbuf);
	}
	else if (strncmp(CtlCmd[0], "flashMemory", sizeof("flashMemory")) == 0)		//读取时间值
	{
		memset(Readbuf, 0, sizeof(Readbuf));
		hfuflash_read(0, Readbuf, sizeof(Readbuf));
		u_printf("The time set is: %s\n", Readbuf);
		
		if (Readbuf[1] == ',')
			SendMsg(CurrentUser, Readbuf);
		else
		{
			memset(Readbuf, 0, sizeof(Readbuf));
			SendMsg(CurrentUser, Readbuf);
		}
	}
}


void USER_FUNC my_main(void)
{
	create_udp();
	InitData();

	//HTTP请求初始化MAC
	HttpInit();

	sleep(10);
	if (-1 == CreateTcpClient())
		u_printf("CreateTcpClient error!\n");
	else if (-1 == ConOpenfire())
	{
		u_printf("ConOpenfire error!\n");

	}

	//创建接收数据线程，只接收用户控制消息
	if (hfthread_create(RecvMsg, "RecvCtlMessage", 512, NULL, HFTHREAD_PRIORITIES_LOW, NULL, NULL) != HF_SUCCESS)
	{
		u_printf("create RecvCtlMessage thread fail\n");
	}
	else
		u_printf("RecvCtlMessage thread start!\n");


	//创建Ping服务器线程
	if (hfthread_create(PingOpenfire, "PingServer", 256, NULL, HFTHREAD_PRIORITIES_LOW, NULL, NULL) != HF_SUCCESS)
	{
		u_printf("create PingServer thread fail\n");
	}
	else
		u_printf("PingServer thread start!\n");


	//创建定时器，设置为自动，每秒count增1
	if((PingTimer = hftimer_create("PingTimer", 1000, true, 1, TimerCallback, 0)) == NULL)
	{
		u_printf("Create PingTimer Fail\n");
	}
	//启动该定时器
	PingCount = 0;
	hftimer_start(PingTimer);

	//创建定时线程
	if (hfthread_create(TimedFeed, "TimedFeed", 256, NULL, HFTHREAD_PRIORITIES_LOW, NULL, NULL) != HF_SUCCESS)
	{
		u_printf("create TimedFeed thread fail\n");
	}

	//校时线程
	if (hfthread_create(correctTime, "correctTime", 256, NULL, HFTHREAD_PRIORITIES_LOW, NULL, NULL) != HF_SUCCESS)
	{
		u_printf("create correctTime thread fail\n");
	}

}


//接收解析服务器消息,在主函数中进行
void USER_FUNC RecvMsg(void* arg)
{
	//第一步：监听接收所有服务器消息
	fd_set Fd;
	char RecvAllBuf[256];
	char *MsgHead = "<message";
	char *PingResult = "type=\"result\"";
	char *CurrentUserStart = "from=";
	char *CurrentUserEnd = "><body>";
	char *ContentBodyStart = "<body>";
	char *ContentBodyEnd = "</body>";
	char ContentBody[30];

	sleep(3);
	GetUser();

	//获取失败则再获取一次
	if (userName[0] == 0){
		sleep(3);
		GetUser();
	}


	//创建喂食线程
	if (hfthread_create(fdthread, "feed", 256, NULL, HFTHREAD_PRIORITIES_LOW, NULL, NULL) != HF_SUCCESS)
	{
		u_printf("create feed thread fail\n");
	}
	
	//创建喂水线程
	if (hfthread_create(wtthread, "water", 256, NULL, HFTHREAD_PRIORITIES_LOW, NULL, NULL) != HF_SUCCESS)
	{
		u_printf("create water thread fail\n");
	}
	
	while(1)
	{
		FD_ZERO(&Fd);
		FD_SET(SocketFd, &Fd);
		int i = hfuart_select(SocketFd+1, &Fd, NULL, NULL, NULL);
		
		if(i <= 0)
			continue;
		if (FD_ISSET(SocketFd, &Fd))
		{
			memset(RecvAllBuf, 0, sizeof(RecvAllBuf));
			read(SocketFd, RecvAllBuf, sizeof(RecvAllBuf));
			// u_printf("RecvAllBuf: %s\n", RecvAllBuf);
			
			//第二步：过滤其他消息, 只响应用户控制消息和Ping服务器回执
			if (strncmp(RecvAllBuf, MsgHead, 8) == 0)
			{
				memset(CurrentUser, 0, sizeof(CurrentUser));
				memset(ContentBody, 0, sizeof(ContentBody));
				
				GetBetweenStrs(RecvAllBuf, ContentBodyStart, ContentBodyEnd, ContentBody);
				GetBetweenStrs(RecvAllBuf, CurrentUserStart, CurrentUserEnd, CurrentUser);
				u_printf("ContentBody :%s\n", ContentBody);
				u_printf("CurrentUser :%s\n", CurrentUser);

				/*if (-1 == LogicHandle(ContentBody, CurrentUser))
					u_printf("CtlSuccess!\n");*/
				LogicHandle(ContentBody);

				u_printf("CtlCmd finished!\n");
				
				//SendMsg(CurrentUser, "CtlSuccess");
			}
			else if(strstr(RecvAllBuf, PingResult) != NULL)
			{
				//收到服务器Ping消息，就把计数置0
				u_printf("Recv Ping Result, The Device Is Online!\n");
				PingCount = 0;
			}
		}

		//usleep(100000);
	}
}


//检测PingCount线程函数
void USER_FUNC CheckPingCount(void* arg)
{
	while(1)
	{
		if (PingCount > 130)
		{
			//执行断线操作
			u_printf("The device is offline, start to reconnect!\n");
			
			PingCount = 0;
			closesocket(SocketFd);
			sleep(10);
			
			hfsys_softreset();
		}
		
		sleep(10);
		
		u_printf("This is CheckPingCount thread\n");
	}
}


int USER_FUNC app_main (void)
{
	//time_t now=time(NULL);
	
	HF_Debug(DEBUG_LEVEL,"sdk version(%s),the app_main start time is %s %s\n",hfsys_get_sdk_version(),__DATE__,__TIME__);
	if(hfgpio_fmap_check(module_type)!=0)
	{
		while(1)
		{
			HF_Debug(DEBUG_ERROR,"gpio map file error\n");
			msleep(1000);
		}
		// return 0;
	}
	//show_reset_reason();
	
	while(!hfnet_wifi_is_active())
	{
		msleep(50);
	}
	#if 0
	int up_result=0;
	up_result = hfupdate_auto_upgrade();
	if(up_result<0)
	{
		u_printf("no entry the auto upgrade mode\n");
	}
	else if(up_result==0)
	{
		u_printf("upgrade success\n");
	}
	else
	{
		u_printf("upgrade fail %d\n",up_result);
	}
	#endif
	
	if(hfnet_start_assis(ASSIS_PORT)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start httpd fail\n");
	}
	
	if (hfnet_start_httpd(HFTHREAD_PRIORITIES_MID) != HF_SUCCESS) 
	{
		HF_Debug(DEBUG_WARN, "start httpd fail\n");
	}

	if ((Huart = hfuart_open(0))<0)
	{
		u_printf("open_uart error!\n");
		exit(1);
	}

	if(hfnet_start_uart(HFTHREAD_PRIORITIES_LOW, (hfnet_callback_t)uart_recv_callback)!=HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start uart fail!\n");
	}
	
	my_main();

	//创建检测全局变量PingCount线程
	hfthread_create(CheckPingCount, "CheckPingCount", 256, NULL, HFTHREAD_PRIORITIES_LOW, NULL, NULL);
	
	
	return 1;
}


///定时线程，需要连接跳线
void USER_FUNC TimedFeed(void *arg)
{
	///取时钟模块时间值
	char GetTimeCmd[4] = {0xFF,0x01,0x03,0x00};
	int  Signal;
	char LastTime[6];

	///定时线程函数中的变量
	char read_timed_flashbuf[160], store_timed_flashbuf[160], timed_cmpbuf[11];
	char *timed_flash_set_str[8], *everyTimeCmd[4];
	char *p_timed_str, *p_cmd;
	int n_timed, k_timed, k_cmd;
	
	memset(LastTime, 0, sizeof(LastTime));
	
	while(1)
	{
		///第一步：读时钟模块上的时间值
		hfuart_send(Huart, GetTimeCmd, sizeof(GetTimeCmd), 0);
		sleep(3);
		
		if (strcmp(RealTime, LastTime) == 0)			///取到不同的时钟值再去执行第二步
		{
			sendto(udp_fd, "same", sizeof("same"), 0, (struct sockaddr *)&sevraddr, sizeof(sevraddr));
			u_printf("same!\n");
		}
		else
		{
			///第二步：取内存中设定的时间值与当前获取的时间值进行匹配
			///	 备注：每次都轮询完flash中所有的时间设置，标识1~4为喂食，标识5~8为喂水，这样set和cancel不用作修改
			memset(read_timed_flashbuf, 0, sizeof(read_timed_flashbuf));
			hfuflash_read(0, read_timed_flashbuf, sizeof(read_timed_flashbuf));
			
			if (read_timed_flashbuf[1] == ',')							///如果不空
			{
				///2.1 分割整个竖线连接的字符串read_timed_flashbuf数组
				p_timed_str = strtok(read_timed_flashbuf, "|");
				n_timed = 0;
				while(p_timed_str)
				{
					timed_flash_set_str[n_timed] = p_timed_str;
					n_timed++;
					p_timed_str = strtok(NULL, "|");
				}
				
				///2.2 匹配时间并喂食，将需要连接的字符串标识存入存储数组中
				k_timed = -1;
				memset(store_timed_flashbuf, 0, sizeof(store_timed_flashbuf));
				while(k_timed < (n_timed - 1))
				{
					k_timed++;
					memset(timed_cmpbuf, 0, sizeof(timed_cmpbuf));
					strcpy(timed_cmpbuf, timed_flash_set_str[k_timed]);				///timed_flash_set_str[0 -  (n_timed-1)]的格式为：1,08:30,1,3
					u_printf("timed_cmpbuf is: %s\n", timed_cmpbuf);
					
					///以","分割timed_cmpbuf
					k_cmd = 0;
					p_cmd = strtok(timed_cmpbuf, ",");
					while(p_cmd)
					{
						everyTimeCmd[k_cmd] = p_cmd;
						k_cmd++;
						p_cmd = strtok(NULL, ",");
					}
					
					///取everyTimeCmd[1]中时间值与时钟模块time_str比较
					if (strcmp(RealTime, everyTimeCmd[1]) == 0)
					{
						sendto(udp_fd, "Got it", sizeof("Got it"), 0, (struct sockaddr *)&sevraddr, sizeof(sevraddr));
						//注：添加定时喂食、喂水的判断
						u_printf("Got it!\n");
						// sendto(udp_fd, LastTime, sizeof(LastTime), 0, (struct sockaddr *)&sevraddr, sizeof(sevraddr));
						// sendto(udp_fd, RealTime, sizeof(RealTime), 0, (struct sockaddr *)&sevraddr, sizeof(sevraddr));
						
						Signal = atoi(everyTimeCmd[0]);
						switch(Signal)
						{
							case 1: case 2: case 3: case 4:
							{
								CtlFeed[3]=(char)(atoi(everyTimeCmd[3]) * 3);
								hfuart_send(Huart, CtlFeed, sizeof(CtlFeed), 0);
								// DataStorage(1, atoi(everyTimeCmd[3]) * 3);
								break;
							}
							case 5: case 6: case 7: case 8:
							{
								CtlWater[3]=(char)(atoi(everyTimeCmd[3]) * 4);
								hfuart_send(Huart, CtlWater, sizeof(CtlWater), 0);
								// DataStorage(0, atoi(everyTimeCmd[3]) * 4);
								break;
							}
							default:
								break;
						}
						
						if (strcmp(everyTimeCmd[2], "1") == 0)
						{
							continue;
						}
					}
					
					///连接字符串
					strcat(store_timed_flashbuf, timed_flash_set_str[k_timed]);
					strcat(store_timed_flashbuf, Vertline);
					
				}
				
				///写进flash中
				hfuflash_erase_page(0,1);
				hfuflash_write(0, store_timed_flashbuf, sizeof(store_timed_flashbuf));
				//sendto(udp_fd, "A496:", sizeof("A496:"), 0, (struct sockaddr *)&address, sizeof(address));
				//sendto(udp_fd, store_timed_flashbuf, sizeof(store_timed_flashbuf), 0, (struct sockaddr *)&address, sizeof(address));
				u_printf("store_timed_flashbuf is: %s\n", store_timed_flashbuf);
			}
		}
		
		//存下当前操作的时间
		memset(LastTime, 0, sizeof(LastTime));
		LastTime[0] = (Clock_H / 10) + 48;
		LastTime[1] = (Clock_H % 10) + 48;
		LastTime[2] = ':';
		LastTime[3] = (Clock_M / 10) + 48;
		LastTime[4] = (Clock_M % 10) + 48;
		
		sleep(15);							///实际时间每10s保证时间更精确，但轮询次数增加
	}
}


//校时线程函数
void USER_FUNC correctTime(void* arg)
{
	char atRecv[64];
	char *ntpStr[3];
	char *ntpTime;
	char timeBuff[10];
	char *hour, *min;
	char ntpBuff[4] = {0xFF,0xDD,0x00,0x00};

	initNTP();
	sleep(60);		//保证收到rtc_time,例:rtc_time=1487170997

	while(1)
	{
		memset(atRecv, 0, sizeof(atRecv));
		hfat_send_cmd("AT+NTPTM\r\n", sizeof("AT+NTPTM\r\n"), atRecv, 64);
		// u_printf("atRecv:%s\n", atRecv);

		//hfat_get_words: 能以逗号、等号、空格、制表符、换行符分割
		if (hfat_get_words(atRecv, ntpStr, 3) > 0) 
		{
			ntpTime = ntpStr[2];		//取出时间,格式为: 15:7:3
			if (strcmp(ntpTime, "Available") == 0)
			{
				u_printf("The time server can't reach.\n");
				sleep(600);				//10分钟后重新取时间
				initNTP();
				continue;
			}

			u_printf("CurrentTime:%s\n", ntpTime);
			//解析时间,发送串口进行校时
			memset(timeBuff, 0, sizeof(timeBuff));
			strcpy(timeBuff, ntpTime);
			hour = strtok(timeBuff, ":");
			min = strtok(NULL, ":");

			ntpBuff[2] = (char)(atoi(hour) / 10 * 16 + atoi(hour) % 10);
			ntpBuff[3] = (char)(atoi(min) / 10 * 16 + atoi(min) % 10);
			hfuart_send(Huart, ntpBuff, sizeof(ntpBuff), 0);
			sendto(udp_fd, "correctTime", sizeof("correctTime"), 0, (struct sockaddr *)&sevraddr, sizeof(sevraddr));
		}

		sleep(2500);		//每8小时校准一次
	}
	
}


//初始化时间服务
void USER_FUNC initNTP(void)
{
	char atRecv[64];

	memset(atRecv, 0, sizeof(atRecv));
	hfat_send_cmd("AT+NTPSER=202.120.2.101\r\n", sizeof("AT+NTPSER=202.120.2.101\r\n"), atRecv, 64);
	u_printf("atRecv:%s\n", atRecv);
	sleep(2);

	memset(atRecv, 0, sizeof(atRecv));
	hfat_send_cmd("AT+NTPEN=ON\r\n", sizeof("AT+NTPEN=ON\r\n"), atRecv, 64);
	u_printf("atRecv:%s\n", atRecv);
	sleep(2);

}


//HTTP的GET方法,用于数据存储
int USER_FUNC DataStorage(int actType, int actTime)
{
	httpc_req_t  http_req;
	http_session_t hhttp = 0;

	char *sendStream_1 = "/wacw/WebServlet?paraName={\"name\":method,\"value\":PhoneControl},";
	char *sendStream_2 = "{\"name\":user,\"value\":";
	char *sendStream_3 = "},{\"name\":mac,\"value\":";
	char *sendStream_4 = "},{\"name\":cmd,\"value\":action#";
	char sendBuf[256];

	char content_data[100];
	int read_size=0;
	int result = 0;
	tls_init_config_t  *tls_cfg=NULL;
	char *test_url = httpURL;
	
	bzero(&http_req, sizeof(http_req));
	http_req.type = HTTP_GET;
	http_req.version = HTTP_VER_1_1;

	if((result = hfhttp_open_session(&hhttp, test_url, 0, tls_cfg, 3)) != HF_SUCCESS)
	{
		u_printf("http open fail\n");
	}

	memset(sendBuf, 0, sizeof(sendBuf));
	strcat(sendBuf, sendStream_1);
	strcat(sendBuf, sendStream_2);
	strcat(sendBuf, userName);
	strcat(sendBuf, sendStream_3);
	strcat(sendBuf, mac);
	strcat(sendBuf, sendStream_4);
	sprintf(sendBuf, "%s%d,%d,", sendBuf, actType, actTime);

	http_req.resource = sendBuf;
	u_printf("sendBuf: %s\n", sendBuf);

	hfhttp_prepare_req(hhttp, &http_req, HDR_ADD_CONN_CLOSE);

	if((result = hfhttp_send_request(hhttp, &http_req)) != HF_SUCCESS)
	{
		u_printf("http send request fail\n");
	}

	memset(content_data, 0, sizeof(content_data));
	while((read_size = hfhttp_read_content(hhttp, content_data, 100)) > 0)
	{
		u_printf("content_data: %s\n", content_data);
		u_printf("read_size: %d\n", read_size);
	}
	
	if(hhttp != 0)
		hfhttp_close_session(&hhttp);

	return result;

}


//HTTP初始化当前用户
int USER_FUNC GetUser(void)
{
	httpc_req_t  http_req;
	http_session_t hhttp = 0;

	char *sendStream_1 = "/wacw/WebServlet?paraName={\"name\":method,\"value\":QueryUserByMac},";
	char *sendStream_2 = "{\"name\":mac,\"value\":";
	char *sendStream_4 = "}";
	char *userHead = "{\'userName\':\'";
	char *userTail = "\'}\"}";
	char sendBuf[256];

	char content_data[100];
	int read_size=0;
	int result = 0;
	tls_init_config_t  *tls_cfg=NULL;
	char *test_url = httpURL;
	
	bzero(&http_req, sizeof(http_req));
	http_req.type = HTTP_GET;
	http_req.version = HTTP_VER_1_1;

	if((result = hfhttp_open_session(&hhttp, test_url, 0, tls_cfg, 3)) != HF_SUCCESS)
	{
		u_printf("http open fail\n");
	}

	memset(sendBuf, 0, sizeof(sendBuf));
	strcat(sendBuf, sendStream_1);
	strcat(sendBuf, sendStream_2);
	strcat(sendBuf, mac);
	strcat(sendBuf, sendStream_4);
	// sprintf(sendBuf, "%s", sendBuf);

	http_req.resource = sendBuf;
	u_printf("sendBuf: %s\n", sendBuf);

	hfhttp_prepare_req(hhttp, &http_req, HDR_ADD_CONN_CLOSE);

	if((result = hfhttp_send_request(hhttp, &http_req)) != HF_SUCCESS)
	{
		u_printf("http send request fail\n");
	}

	memset(content_data, 0, sizeof(content_data));
	while((read_size = hfhttp_read_content(hhttp, content_data, 100)) > 0)
	{
		memset(userName, 0, sizeof(userName));
		// u_printf("GetUserData: %s\n", content_data);
		u_printf("DataSize: %d\n", read_size);
		GetBetweenStrs(content_data, userHead, userTail, userName);
		u_printf("userName: %s\n", userName);
	}
	
	if(hhttp != 0)
		hfhttp_close_session(&hhttp);

	return result;

}


//在发送HTTP请求前初始化,获取MAC
void USER_FUNC HttpInit(void)
{
	char *Mac;
	char *MacStr[3];
	char rsp[64];
	memset(rsp, 0, sizeof(rsp));

	hfat_send_cmd("AT+WSMAC\r\n", sizeof("AT+WSMAC\r\n"), rsp, 64);

	//hfat_get_words: 能以逗号、等号、空格、制表符、换行符分割
	if (hfat_get_words(rsp, MacStr, 3) > 0) 
	{
		Mac = MacStr[1];						//Mac赋值
		memset(mac, 0, sizeof(mac));
		strcpy(mac, Mac);
	}

}


//串口回调,更新时钟
static int USER_FUNC uart_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len)
{
	// HF_Debug(DEBUG_LEVEL_LOW,"[%d]uart recv %d bytes data %d\n",event,len,buf_len);
	int temp = 0;

	temp = BCDToInt(data[0]);
	if (strcmp(data, "fd") == 0 || temp == 66)
	{
		DataStorage(1, 3);
		sendto(udp_fd, data, sizeof(data), 0, (struct sockaddr *)&sevraddr, sizeof(sevraddr));
	}
	else if (strcmp(data, "wt") == 0 || temp == 77)
	{
		DataStorage(0, 4);
		sendto(udp_fd, data, sizeof(data), 0, (struct sockaddr *)&sevraddr, sizeof(sevraddr));
	}
	else if (temp < 24)		//防止将f(66)及w(77)当成时间进行更新
	{	
		Clock_H = BCDToInt(data[0]);			 ///将BCD码转成十进制的时
		Clock_M = BCDToInt(data[1]);

		memset(RealTime, 0, sizeof(RealTime));
		RealTime[0] = (Clock_H / 10) + 48;              ///数字到字符
		RealTime[1] = (Clock_H % 10) + 48;
		RealTime[2] = ':';
		RealTime[3] = (Clock_M / 10) + 48;
		RealTime[4] = (Clock_M % 10) + 48;

		sendto(udp_fd, RealTime, sizeof(RealTime), 0, (struct sockaddr *)&sevraddr, sizeof(sevraddr));
	}
	else
		sendto(udp_fd, data, sizeof(data), 0, (struct sockaddr *)&sevraddr, sizeof(sevraddr));

	return len;
}


//bcd码到十进制int, 例如：char 0x14转换为int 14
int USER_FUNC BCDToInt(char bcd)
{
    return (0xff & (bcd>>4))*10 +(0xf & bcd);
}


//测试UDP
int USER_FUNC create_udp(void)
{
	if((udp_fd = socket(AF_INET,SOCK_DGRAM,0)) == -1)
	{
		u_printf("create socket error!\n");
		exit(1);
	}
	
	bzero(&sevraddr, sizeof(sevraddr));
	sevraddr.sin_family=AF_INET;
	sevraddr.sin_addr.s_addr=inet_addr("115.28.179.114");
	sevraddr.sin_port=htons(8018);
	
	return udp_fd;

}


#endif


