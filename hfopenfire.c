#include <hfopenfire.h>


//����TCP����
int USER_FUNC CreateTcpClient(void)
{
	int result;
	struct sockaddr_in sevraddr;
	
	SocketFd = socket(AF_INET,SOCK_STREAM,0);
	
	bzero(&sevraddr,sizeof(sevraddr));
	sevraddr.sin_family=AF_INET;
	sevraddr.sin_addr.s_addr=inet_addr(ServerIp);
	sevraddr.sin_port=htons(ServerPort);
	
	sleep(5);            				//���豸һ��ʱ�Ӳ�������tcp
	result = connect(SocketFd, (struct sockaddr *)&sevraddr, sizeof(sevraddr));

	if (result == -1)
		u_printf("connect error!\n");

	return result;
	
}


//��ʼ�����ӷ���������
void USER_FUNC InitData(void)
{
	char Base64SrcStr[64];
	
	//��ȡMac
	char *Mac;
	char *MacStr[3];
	char rsp[64];
	memset(rsp, 0, sizeof(rsp));
	hfat_send_cmd("AT+WSMAC\r\n", sizeof("AT+WSMAC\r\n"), rsp, 64);

	//hfat_get_words: ���Զ��š��Ⱥš��ո��Ʊ�������з��ָ�
	if (hfat_get_words(rsp, MacStr, 3) > 0) 
	{
		Mac = MacStr[1];						//Mac��ֵ
	}
	
	memset(SmartHostFjid, 0, sizeof(SmartHostFjid));
	strcat(SmartHostFjid, Mac);					//ȫ��SmartHostFijd��ֵ
	strcat(SmartHostFjid, HostFjid);
	u_printf("SmartHostFjid: %s\n", SmartHostFjid);
	
	
	// ��Base64SrcStr��ֵ
	char *Pwd = "rtyz";
	int userLen = strlen(Mac);
	int pswLen = strlen(Pwd);
	memset(Base64SrcStr, 0, sizeof(Base64SrcStr));
	Base64SrcStr[0]='\0';
	int i;
	for(i=0; i<userLen; i++)
	{
		Base64SrcStr[i+1] = Mac[i];
	}
	
	int j=i+1;
	Base64SrcStr[j]='\0';
	for(i=0; i<pswLen; i++)
	{
		Base64SrcStr[j+1+i] = Pwd[i];
	}
	Base64SrcStr[j+1+i]='\0';
	
	// ��Base64Des��ֵ
	int srcStrLen = 2 + strlen(Mac) + strlen(Pwd);
	memset(Base64Des, 0, sizeof(Base64Des));
	ConvertToBase64(Base64Des, Base64SrcStr, srcStrLen);		//���þֲ�����Base64Des
	u_printf("Base64Des: %s\n", Base64Des);
	
}


//����Openfire������
int USER_FUNC ConOpenfire(void)
{
	
	char *OpenStream_1 = "<stream:stream to='";
	char *OpenStream_2 = "' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>";

	char *AuthXml_1 = "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>";
	char *AuthXml_2 = "</auth>";

	char *ResourceBind = "<iq id='smart' type='set'><bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'><resource>WSJ</resource></bind></iq>";

	char *GetSession = "<iq xmlns='jabber:client' id='smart' type='set'><session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>";

	char *OnlineStatus = "<presence id='smart' type='available'><status>Online</status><priority>1</priority></presence>";
	
	char OpenTmpBuf[256];
	
	int result;
	//��һ��������OpenStream
	memset(OpenTmpBuf, 0, sizeof(OpenTmpBuf));
	strcat(OpenTmpBuf, OpenStream_1);
	strcat(OpenTmpBuf, ServerName);
	strcat(OpenTmpBuf, OpenStream_2);
	result = send(SocketFd, OpenTmpBuf, sizeof(OpenTmpBuf), 0);
	if (result < 0)
		u_printf("send1 error\n");
	sleep(2);
	
	//�ڶ���������AuthXml
	memset(OpenTmpBuf, 0, sizeof(OpenTmpBuf));
	strcat(OpenTmpBuf, AuthXml_1);
	strcat(OpenTmpBuf, Base64Des);
	strcat(OpenTmpBuf, AuthXml_2);
	result = send(SocketFd, OpenTmpBuf, sizeof(OpenTmpBuf), 0);
	if (result < 0)
		u_printf("send2 error\n");
	sleep(2);
	
	//��������ͬ��һ��
	memset(OpenTmpBuf, 0, sizeof(OpenTmpBuf));
	strcat(OpenTmpBuf, OpenStream_1);
	strcat(OpenTmpBuf, ServerName);
	strcat(OpenTmpBuf, OpenStream_2);
	result = send(SocketFd, OpenTmpBuf, sizeof(OpenTmpBuf), 0);
	if (result < 0)
		u_printf("send3 error\n");
	sleep(2);
	
	//���Ĳ�������ResourceBind
	memset(OpenTmpBuf, 0, sizeof(OpenTmpBuf));
	strcat(OpenTmpBuf, ResourceBind);
	result = send(SocketFd, OpenTmpBuf, sizeof(OpenTmpBuf), 0);
	if (result < 0)
		u_printf("send4 error\n");
	sleep(2);
	
	//���岽������GetSession
	memset(OpenTmpBuf, 0, sizeof(OpenTmpBuf));
	strcat(OpenTmpBuf, GetSession);
	result = send(SocketFd, OpenTmpBuf, sizeof(OpenTmpBuf), 0);
	if (result < 0)
		u_printf("send5 error\n");
	sleep(2);
	
	//������������OnlineStatus
	memset(OpenTmpBuf, 0, sizeof(OpenTmpBuf));
	strcat(OpenTmpBuf, OnlineStatus);
	result = send(SocketFd, OpenTmpBuf, sizeof(OpenTmpBuf), 0);
	if (result < 0)
		u_printf("send6 error\n");
	

	u_printf("ConOpenfire Success!\n");
	return 0;
	
}


//Ping������
void USER_FUNC PingOpenfire(void* arg)
{
	char OpenTmpBuf[256];
	char *PingServer_1 = "<iq from='";
	char *PingServer_2 = "' to='";
	char *PingServer_3 = "' id='smart' type='get'><ping xmlns='urn:xmpp:ping'/></iq>";
	
	while(1)
	{
		sleep(81);			//ÿ����Ping������һ��
		
		memset(OpenTmpBuf, 0, sizeof(OpenTmpBuf));
		strcat(OpenTmpBuf, PingServer_1);
		strcat(OpenTmpBuf, SmartHostFjid);
		strcat(OpenTmpBuf, PingServer_2);
		strcat(OpenTmpBuf, ServerName);
		strcat(OpenTmpBuf, PingServer_3);
		send(SocketFd, OpenTmpBuf, sizeof(OpenTmpBuf), 0);
		
	}

}


//��ʱ���ص�
void USER_FUNC TimerCallback(hftimer_handle_t timer)
{
	PingCount++;
}


//����ǰ�û��ظ���Ϣ
void USER_FUNC SendMsg(char *CurrentUser, char *Content)
{
	char *MsgOut_1 = "<message id='smart' from='";
	char *MsgOut_2 = "' to=";
	char *MsgOut_3 = " type='chat'><body>";
	char *MsgOut_4 = "</body><thread>123</thread></message>";
	char OpenTmpBuf[256];
	
	memset(OpenTmpBuf, 0, sizeof(OpenTmpBuf));
	strcat(OpenTmpBuf, MsgOut_1);
	strcat(OpenTmpBuf, SmartHostFjid);
	strcat(OpenTmpBuf, MsgOut_2);
	strcat(OpenTmpBuf, CurrentUser);
	strcat(OpenTmpBuf, MsgOut_3);
	strcat(OpenTmpBuf, Content);
	strcat(OpenTmpBuf, MsgOut_4);
	send(SocketFd, OpenTmpBuf, sizeof(OpenTmpBuf), 0);
}


//ȡ�����ַ����м������
void USER_FUNC GetBetweenStrs(char* Msg, char* MsgStart, char* MsgEnd, char* MsgBuf)
{
	char *str[2];
	int ContentLen;

	str[0] = strstr(Msg, MsgStart);					//<body>Pour#1</body></message>, 29
	str[1] = strstr(Msg, MsgEnd);					//</body></message>, 17
	ContentLen = strlen(str[0]) - strlen(str[1]) - strlen(MsgStart);

	strncpy(MsgBuf, str[0] + strlen(MsgStart), ContentLen);
}


//���ߺ�����base64����
long USER_FUNC ConvertToBase64(char *pcOutStr, const char *pccInStr, int iLen)
{
	static const char g_ccB64Tbl[64] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   
    const char* pccIn = (const char *)pccInStr;
    char* pcOut;
    int iCount;
    pcOut = pcOutStr;

	if(!pcOutStr || !pccInStr)
	{
		return -1;
	}

    //Loop in for Multiple of 24Bits and Convert to Base 64    
    for (iCount = 0; iLen - iCount >=3 ; iCount += 3,pccIn += 3)
    {       
        *pcOut++ = g_ccB64Tbl[pccIn[0] >> 2];
        *pcOut++ = g_ccB64Tbl[((pccIn[0] & 0x03)<<4) | (pccIn[1] >> 4)];
        *pcOut++ = g_ccB64Tbl[((pccIn[1] & 0x0F)<<2) | (pccIn[2] >> 6)];
        *pcOut++ = g_ccB64Tbl[pccIn[2] & 0x3f];
		
    }
	
    //Check if String is not multiple of 3 Bytes
    if (iCount != iLen)
    {
        
        char ucLastByte;

        *pcOut++ = g_ccB64Tbl[pccIn[0] >> 2];
        ucLastByte = ((pccIn[0] & 0x03)<<4);
        
      
        if (iLen - iCount > 1)
        {
            //If there are 2 Extra Bytes
            ucLastByte |= (pccIn[1] >> 4);
            *pcOut++ = g_ccB64Tbl[ucLastByte];
            *pcOut++ = g_ccB64Tbl[((pccIn[1] & 0x0F)<<2)];
        }
        else
        {
             //If there is only 1 Extra Byte
             *pcOut++ = g_ccB64Tbl[ucLastByte];
             *pcOut++ = '=';
        }            
                                
         *pcOut++ = '=';            
    }
	
    *pcOut  = '\0';
	return 0;
	
}


///////////////////////ιʳ����غ�������
//���ö�ʱ
/*void USER_FUNC SetTimer(char *CtlCmd)
{
	char *pSnd;
	char *SetStr[8];
	char Readbuf[120], Storebuf[120];
	char SetTmp[11], CmdTmp[11];
	int  nSnd, kInt, cInt;

	memset(Readbuf, 0, sizeof(Readbuf));
	hfuflash_read(0, Readbuf, sizeof(Readbuf));
	//memset(SetStr, 0, sizeof(SetStr));
	
	if (Readbuf[1] == ',') 					//flash����ֵ
	{
		///��һ�������߷ָ�flash��ʱ��ֵ������ָ������SetStr��
		pSnd = strtok(Readbuf, "|");
		nSnd = 0;
		while(pSnd)
		{
			//strcpy(SetStr[nSnd], pSnd);
			SetStr[nSnd] = pSnd;
			nSnd++;
			pSnd = strtok(NULL, "|");
		}
		
		///�ڶ�����ȡSetStrֵ�б�ʶ��cmd[1]���бȽ�
		
		kInt = nSnd - 1;
		while(kInt >= 0)
		{
			memset(SetTmp, 0, sizeof(SetTmp));
			strcpy(SetTmp, SetStr[kInt]);               ///strcpy����
			u_printf("The SetTmp is: %s\n", SetTmp);
			
			memset(CmdTmp, 0, sizeof(CmdTmp));
			strcpy(CmdTmp, CtlCmd);
			u_printf("The CmdTmp is: %s\n", CmdTmp);
			
			if (strncmp(strtok(CmdTmp, ","), strtok(SetTmp, ","), 1) == 0)        ///�Ƚϱ�ʶ
			{
				// strcpy(SetStr[kInt], CtlCmd);
				SetStr[kInt] = CtlCmd;
				break;
			}
			
			kInt--;
		}
		
		///�������������ַ�����ŵ�Storebuf��
		memset(Storebuf, 0, sizeof(Storebuf));
		if (kInt < 0)								//û�б�ʶ�����
		{
			cInt = 0;
			while(cInt < nSnd)
			{
				strcat(Storebuf, SetStr[cInt]);
				strcat(Storebuf, Vertline);
				cInt++;
			}
			strcat(Storebuf, CtlCmd);
		}
		else										//�޸�����
		{
			cInt = 0;
			while(cInt < nSnd)
			{
				strcat(Storebuf, SetStr[cInt]);
				if (cInt < (nSnd - 1))
					strcat(Storebuf, Vertline);
				cInt++;
			}
		}
	}
	else									//flashΪ��
	{
		memset(Storebuf, 0, sizeof(Storebuf));
		strcat(Storebuf, CtlCmd);
	}
	
	//���Ĳ����洢
	hfuflash_erase_page(0, 1);
	hfuflash_write(0, Storebuf, sizeof(Storebuf));
	u_printf("We have stored the Storebuf: %s\n", Storebuf);

	SendMsg(CurrentUser, "CtlSuccess");
}*/


void USER_FUNC SetTimer(char *CtlCmd)
{
	char read_flashbuf[160], store_flashbuf[160];             //��ȡ���洢�û�flash�Ļ�������
	char tmp_flash_strcmp[11], tmp_cmd_strcmp[11];
	char *flash_set_str[8];
	char *p_str;
	int n_str, k_str, c_str;

	memset(read_flashbuf, 0, sizeof(read_flashbuf));
	hfuflash_read(0, read_flashbuf, sizeof(read_flashbuf));
	
	if (read_flashbuf[1] == ',') 					//flash����ֵ
	{
		///��һ�������߷ָ�flash��ʱ��ֵ������ָ������flash_set_str��
		p_str = strtok(read_flashbuf, "|");
		n_str = 0;
		while(p_str)
		{
			flash_set_str[n_str] = p_str;
			n_str++;
			p_str = strtok(NULL, "|");
		}
		
		///�ڶ�����ȡflash_set_strֵ�б�ʶ��cmd[1]���бȽ�
		
		k_str = n_str - 1;
		while(k_str >= 0)
		{
			memset(tmp_flash_strcmp, 0, sizeof(tmp_flash_strcmp));
			strcpy(tmp_flash_strcmp, flash_set_str[k_str]);               ///strcpy����
			u_printf("The tmp_flash_strcmp is: %s\n", tmp_flash_strcmp);
			
			memset(tmp_cmd_strcmp, 0, sizeof(tmp_cmd_strcmp));
			strcpy(tmp_cmd_strcmp, CtlCmd);
			u_printf("The tmp_cmd_strcmp is: %s\n", tmp_cmd_strcmp);
			
			if (strncmp(strtok(tmp_cmd_strcmp, ","), strtok(tmp_flash_strcmp, ","), 1) == 0)        ///�Ƚ�ʱ��ı�ʶ
			{
				// strcpy(flash_set_str[k_str], cmd[1]);
				flash_set_str[k_str] = CtlCmd;
				
				break;
			}
			
			k_str--;
		}
		
		///�������������ַ�����ŵ�store_flashbuf��
		memset(store_flashbuf, 0, sizeof(store_flashbuf));
		if (k_str < 0)								//flash��û�иñ�ʶ������ʾ���
		{
			c_str = 0;
			while(c_str < n_str)
			{
				strcat(store_flashbuf, flash_set_str[c_str]);
				strcat(store_flashbuf, Vertline);
				c_str++;
			}
			strcat(store_flashbuf, CtlCmd);
		}
		else										//�޸�ĳ����ʶ������
		{
			c_str = 0;
			while(c_str < n_str)
			{
				strcat(store_flashbuf, flash_set_str[c_str]);
				if (c_str < (n_str - 1))
					strcat(store_flashbuf, Vertline);
				c_str++;
			}
		}
	}
	else		///flash��Ϊ��
	{
		memset(store_flashbuf, 0, sizeof(store_flashbuf));
		strcat(store_flashbuf, CtlCmd);
	}
	
	///���Ĳ����洢���û�flash��
	hfuflash_erase_page(0, 1);
	hfuflash_write(0, store_flashbuf, sizeof(store_flashbuf));
	u_printf("We have stored the store_flashbuf: %s\n", store_flashbuf);

	SendMsg(CurrentUser, "CtlSuccess");
}


//ȡ����ʱ
/*void USER_FUNC CancelTimer(char *CtlCmd)
{
	char *pSnd;
	char *SetStr[8];
	char Readbuf[120], Storebuf[120];
	char SetTmp[11];
	int  nSnd, kInt, cInt;

	memset(Readbuf, 0, sizeof(Readbuf));
	hfuflash_read(0, Readbuf, sizeof(Readbuf));
	//memset(SetStr, 0, sizeof(SetStr));
	
	if (Readbuf[1] == ',') 				//flash����ֵ
	{
		///��һ�������߷ָ�flash��ʱ��ֵ������ָ������SetStr��
		pSnd = strtok(Readbuf, "|");
		nSnd = 0;
		while(pSnd)
		{
			//strcpy(SetStr[nSnd], pSnd);
			SetStr[nSnd] = pSnd;
			nSnd++;
			pSnd = strtok(NULL, "|");
		}
		
		///�ڶ�����ȡSetStrֵ�б�ʶ��cmd[1]���бȽ�
		kInt = 0;
		while(kInt < nSnd)
		{
	        memset(SetTmp, 0, sizeof(SetTmp));
	        strcpy(SetTmp, SetStr[kInt]);
			
			if (strncmp(strtok(SetTmp, ","), CtlCmd, 1) == 0)
			{
				break;
			}
			
			kInt++;
		}
		
		///��������SetStr[kInt]��ΪҪɾ���Ķ�ʱ�ַ�������������ķŵ�store_flashbuf��
		cInt = 0;
		memset(Storebuf, 0, sizeof(Storebuf));
		
		while(cInt < nSnd) 
		{
			if(cInt == kInt)
			{
				cInt++;
				continue;
			}
			if (kInt < (nSnd -1))
			{
				strcat(Storebuf, SetStr[cInt]);
				if (cInt < (nSnd - 1))
					strcat(Storebuf, Vertline);
				cInt++;
			}
			else
			{
				strcat(Storebuf, SetStr[cInt]);
				if (cInt < (nSnd - 2))
					strcat(Storebuf, Vertline);
				cInt++;
			}
		}
		
		///���Ĳ����洢���û�flash��
		hfuflash_erase_page(0,1);
		hfuflash_write(0, Storebuf, sizeof(Storebuf));
		u_printf("We have stored the Storebuf: %s\n", Storebuf);
		
		SendMsg(CurrentUser, "CtlSuccess");
	}
}*/


void USER_FUNC CancelTimer(char *CtlCmd)
{
	char read_flashbuf[160], store_flashbuf[160];             //��ȡ���洢�û�flash�Ļ�������
	char tmp_flash_strcmp[11];
	char *flash_set_str[8];
	char *p_str;
	int n_str, k_str, c_str;

	memset(read_flashbuf, 0, sizeof(read_flashbuf));
	hfuflash_read(0, read_flashbuf, sizeof(read_flashbuf));
	
	if (read_flashbuf[1] == ',') 				//flash����ֵ
	{
		///��һ�������߷ָ�flash��ʱ��ֵ������ָ������flash_set_str��
		p_str = strtok(read_flashbuf, "|");
		n_str = 0;
		while(p_str)
		{
			flash_set_str[n_str] = p_str;
			n_str++;
			p_str = strtok(NULL, "|");
		}
		
		///�ڶ�����ȡflash_set_strֵ�б�ʶ��cmd[1]���бȽ�
		k_str = 0;
		while(k_str < n_str)
		{
	        memset(tmp_flash_strcmp, 0, sizeof(tmp_flash_strcmp));
	        strcpy(tmp_flash_strcmp, flash_set_str[k_str]);
			
			if (strncmp(strtok(tmp_flash_strcmp, ","), CtlCmd, 1) == 0)
			{
				break;
			}
			
			k_str++;
		}
		
		///��������flash_set_str[k_str]��ΪҪɾ���Ķ�ʱ�ַ�������������ķŵ�store_flashbuf��
		c_str = 0;
		memset(store_flashbuf, 0, sizeof(store_flashbuf));
		
		while(c_str < n_str) 
		{
			if(c_str == k_str)
			{
				c_str++;
				continue;
			}
			if (k_str < (n_str -1))
			{
				strcat(store_flashbuf, flash_set_str[c_str]);
				if (c_str < (n_str - 1))
					strcat(store_flashbuf, Vertline);
				c_str++;
			}
			else
			{
				strcat(store_flashbuf, flash_set_str[c_str]);
				if (c_str < (n_str - 2))
					strcat(store_flashbuf, Vertline);
				c_str++;
			}
		}
		
		///���Ĳ����洢���û�flash��
		hfuflash_erase_page(0,1);
		hfuflash_write(0, store_flashbuf, sizeof(store_flashbuf));
		u_printf("We have stored the store_flashbuf: %s\n", store_flashbuf);
		
		SendMsg(CurrentUser, "CtlSuccess");
	}
}


///ιʳ�߳�
void USER_FUNC fdthread(void *arg)
{
	while(1)
	{
		if (fdstate == 1)
		{
			u_printf("Start to feed %d secs!\n", atoi(ActCmd[1]));
			
			CtlFeed[3]=(char)(atoi(ActCmd[1]));
			if (hfuart_send(Huart, CtlFeed, sizeof(CtlFeed), 0) > 0)
			{
				SendMsg(CurrentUser, "CtlSuccess");
			}
			
			sleep((int)(atoi(ActCmd[1]) / 0.7343));
			fdstate = 0;
			u_printf("Feeding has finished!\n");
		}
		
		msleep(100);
	}
}


///ιˮ�߳�
void USER_FUNC wtthread(void *arg)
{
	while(1)
	{
		if (wtstate == 1)
		{
			u_printf("Start to water %d secs!\n", atoi(ActCmd[1]));
			
			CtlWater[3]=(char)(atoi(ActCmd[1]));
			if (hfuart_send(Huart, CtlWater, sizeof(CtlWater), 0) > 0)
			{
				SendMsg(CurrentUser, "CtlSuccess");
			}

			sleep((int)(atoi(ActCmd[1]) / 0.7343));
			wtstate = 0;
			u_printf("Watering has finished!\n");
		}
		
		msleep(100);
	}
}


