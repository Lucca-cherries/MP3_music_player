#include <stdio.h>
#include <io.h>
#include <Windows.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <conio.h>
#include <mmsystem.h>
#include <time.h>
#include <stdarg.h>
#define LENGTH 500

#pragma comment(lib,"winmm.lib")
HANDLE hOutput, hOutBuf, hSet;                   //控制台屏幕缓冲区句柄
COORD coord;
DWORD bytes = 0;
char PrintfBuff[LENGTH];
HANDLE  hOut[2];                                 //两个缓冲区的句柄
HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
//获取系统默认缓冲区句柄



enum { FATAL, ERROR_, WARNING, INFO, DEBUG };    //log日志记录类型 一共分FATAL ERROR_ WARNING INFO DEBUG 五种


//播放列表对应链表的定义
typedef struct MusicNode {
	int ReId;                 //相对ID，满足1~n严格递增
	char name[100];           //歌曲名称
	struct MusicNode* next;   //next指针
}MusicNode, * LinkNode;


struct lrc {
	int time;                 //每句歌词对应歌词时间
	char lyric[100];          //歌词内容
}LRC[1000];                                      //LRC[i].lyric表示第IDInput首歌的第i行歌词
												 //同一时间只读取一首歌的歌词


int LinesNumber;                                 //LinesNumber表示第IDInput首歌的总行数

LinkNode head[100] = { NULL };                   //100个歌单初始化为空链表
const char* Mode[4] = { "0", "顺序播放","单曲循环","随机播放" };

char _PATH[100] = { NULL };                      //用户自定义mp3文件以及歌单txt文件储存路径
char alls[1000][100];                            //库中所有歌曲名称用alls储存，带后缀
char listname[100][100];                         //自定义播放列表名称，输入时不带后缀，储存时带后缀         
												 //新建歌单需要输入listname,通过addone添加歌曲

int ListSize[100];                               //ListSize[i]表示第i个歌单有多少首歌
int ListNumber = 0;                              //歌单的数量

int MusicNumber = 0;                             //库中歌曲个数

int Mci_Flag = 0;                                //判断是否调用MusicOpen 0表示未调用函数
int Bro_Flag = -1;                               //播放/暂停 0表示暂停 1表示播放 -1表示未打开音乐
int ReId = -1;                                   //在当前歌单中的相对id
int ListFlag = -1;                               //当前播放歌曲所在歌单的编号
int BackFlag = -1;                               //用于解决返回主菜单时音乐无法播放的问题
int ModeFlag = 1;                                //播放模式 1顺序播放（默认) 2单曲循环 3随机播放
int p = 1;                                       //用于确定当前歌词行数，关闭音乐时置1代表下一次从歌词第一行开始显示
int LyricFlag = 1;                               //遍历p，将值赋给LyricFlag，记录当前是第几行歌词
char BroName[100];                               //当前播放的歌曲名称
int IDInput = -1;                                //要播放的歌曲在当前列表的序号
int LyricShowFlag;                               //0表示当前打开的这首歌没有lrc，1表示有
bool BuffSwitch;                                 //缓冲区转换标志


//日志记录函数
void log_record(int error_level, const char* format, ...)
{
	va_list args;
	FILE* fp = NULL;
	char time_str[32];
	char file_name[256];

	va_start(args, format);
	time_t time_log = time(NULL);
	strftime(file_name, sizeof(file_name), "%Y-%m-%d_log_history.log", localtime(&time_log));


	if ((fp = fopen(file_name, "a+")) != NULL)
	{
		strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&time_log));
		if (error_level == (int)FATAL)
		{
			fprintf(fp, "[%s]-[%s]-[%s] :>   ", time_str, "FATAL", __FILE__);
			vfprintf(fp, format, args);
			fprintf(fp, "\n");

		}
		if (error_level == (int)ERROR_)
		{
			fprintf(fp, "[%s]-[%s]-[%s] :>   ", time_str, "ERROR", __FILE__);
			vfprintf(fp, format, args);
			fprintf(fp, "\n");

		}
		else if (error_level == (int)WARNING)
		{
			fprintf(fp, "[%s]-[%s]-[%s] :> ", time_str, "WARNINGs", __FILE__);
			vfprintf(fp, format, args);
			fprintf(fp, "\n");
		}
		else if (error_level == (int)INFO)
		{
			fprintf(fp, "[%s]-[%s]-[%s] :>      ", time_str, "INFO", __FILE__);
			vfprintf(fp, format, args);
			fprintf(fp, "\n");
		}
		else if (error_level == (int)DEBUG)
		{
			fprintf(fp, "[%s]-[%s]-[%s] :>    ", time_str, "DEBUG", __FILE__);
			vfprintf(fp, format, args);
			fprintf(fp, "\n");
		}
		fclose(fp);
	}

	va_end(args);
}



void initialalls();                              //扫描本地mp3文件，将歌名信息储存到alls数组中

												 //将字符数组path存入alls，同时更新MusicNumber
inline void addalls(const char* path)
{
	strcpy(alls[++MusicNumber], path);
}
void addone(const char* name, int ListFlag);     //将名称为name的歌曲，添加到head[LishFlag]尾部
void PrintList(int ListFlag);                    //将head[ListFlag]中的歌曲打印
void SaveList(int ListFlag);                     //启动程序时初始化歌单 程序结束时再保存歌单，遍历链表，覆盖原txt文档
char* GetName(int ReId, int ListFlag);           //返回head[ListFlag]的第ReId首歌的名称，保证ReId不越界
void initiallist();                              //打开程序时读取txt，创建list
void addlist(int n, const char* txtname);        //从txtname名称的文件读取信息存到head[n]中
void deleteone(int n, int ListFlag);             //删除head[ListFlag]中第n首歌
char* GetPrev();                                 //保证ReId不越界，返回上一首歌的名称，每次返回时用strcpy传给另一个字符数组
char* GetNext();                                 //保证ReId不越界，返回下一首歌的名称，不改变ReId,IDInput等的值
bool exist(const char* path);                    //判断名称相对路径为path的文件是否存在
void Readlrc();                                  //根据当前的IDInput和ListFlag读取歌词信息，存入LRC结构体
int DeleteList(int n);                           //删除编号为n的歌单，删除成功返回0，否则返回-1
void Play();

//根据歌曲当前播放进度，用字符数组储存歌曲播放进度条的显示样例，返回该数组的首地址
char* Getprogress(int music_alltime, int music_timeing_ms);
char* Getvolumeline(int vol);                    //用字符数组储存当前音量的显示样例，返回该数组的首地址



/*********************************************************
实现一些与mci相关的函数
*********************************************************/

//表示链接xxx.lib这个库，告诉编译器你要用到xxx.lib库
//和在工程设置里写上链入xxx.lib的效果一样，
//不过这种方法写的 程序别人在使用你的代码的时候就不用再设置工程settings了。

#define  CTRLCMD_SIZE 256

//打开设备
void MusicOpen(const char* pPath)
{
	char CtrlCmd[CTRLCMD_SIZE];

	char pathindex[100] = { NULL };
	strcpy(pathindex, _PATH);

	strcat(pathindex, pPath);
	sprintf(CtrlCmd, "open \"%s\" alias mymusic", pathindex);//alias别名
	mciSendString(CtrlCmd, NULL, 0, NULL);                   //向一个MCI的设备驱动程序发送一个字符串
	Mci_Flag = 1;
}

//播放音乐
void MusicPlay()
{
	mciSendString("setaudio mymusic volume to 500", NULL, 0, NULL);
	mciSendString("play mymusic", NULL, 0, NULL);
}

//停止音乐
void MusicStop()
{
	mciSendString("stop mymusic", NULL, 0, NULL);
}

//关闭音乐
void MusicClose()
{
	char c[100];
	int i = mciSendString("close mymusic", NULL, 0, NULL);//函数成功调用，则返回0，调用失败返回错误代码
	if (i)                                                //调用函数失败
	{
		mciGetErrorString(i, c, 100);                     //用于返回一个错误代码的文本描述，C是用来接收返回的文本描述的缓冲区指针
		printf("%s\n", c);
	}
	Mci_Flag = 0;
}

//暂停音乐
void MusicPause()
{
	char c[100];
	int i = mciSendStringA("pause mymusic", NULL, 0, NULL);
	if (i)
	{
		mciGetErrorString(i, c, 100);
		printf("%s\n", c);
	}
}

//返回当前音量大小
int MusicGetVolume(void)
{
	char c[100];
	int v0 = 0;
	mciSendString("status mymusic volume", c, sizeof(c), NULL);
	v0 = atoi(c);    //c一个远指针，它指向由应用程序返回的字符串缓冲区，sizeof(c)表示缓冲区的大小
	return v0;       //atoi把字符串转成整型，参数是const char* 但是音量在哪里？为什么一定字符串里面是数字字符？
}

//增加音量
void MusicSetVolumeUp(void)
{
	int volume = 0;
	char cmd[100];
	volume = MusicGetVolume();
	if (volume + 100 <= 1000)
	{
		wsprintf(cmd, "setaudio mymusic volume to %d", volume + 100);//将一系列的字符和数值输入到缓冲区
		mciSendString(cmd, NULL, 0, NULL);
	}
	else
	{
		mciSendString("setaudio mymusic volume to 1000", NULL, 0, NULL);
	}
}

//减小音量
void MusicSetVolumeDown(void)
{
	int volume = 0;
	char cmd[100];
	volume = MusicGetVolume();
	if (volume - 100 >= 0)
	{
		wsprintf(cmd, "setaudio mymusic volume to %d", volume - 100);//将一系列的字符和数值输入到缓冲区
		mciSendString(cmd, NULL, 0, NULL);
	}
	else
	{
		mciSendString("setaudio mymusic volume to 0", NULL, 0, NULL);
	}
}

//快进5秒
void MusicFastForward(void)
{
	char c[100];
	char cmd[100];
	char c1[100];
	int iCurrent, i;
	mciSendString("pause mymusic", NULL, 0, NULL);
	mciSendString("status mymusic position", c, sizeof(c), NULL);
	iCurrent = atoi(c);                                      //当前播放位置

	wsprintf(cmd, "seek mymusic to %d", iCurrent + 5000);
	i = mciSendString(cmd, NULL, 0, NULL);
	if (i)
	{
		mciGetErrorString(i, c1, 100);
		printf("%s\n", c1);
	}
	else   printf("%d\n", mciSendString("play mymusic", NULL, 0, NULL));
}

//快退5秒 默认快进快退合法
void MusicFastBackward(void)
{
	char c[100];
	char cmd[100];
	char c1[100];
	int iCurrent, i;
	mciSendString("pause mymusic", NULL, 0, NULL);
	mciSendString("status mymusic position", c, sizeof(c), NULL);//用atoi表示当前位置是提取音乐序号？
	iCurrent = atoi(c);                                          //当前播放位置

	wsprintf(cmd, "seek mymusic to %d", iCurrent - 5000);        //当前播放位置-5000
	i = mciSendString(cmd, NULL, 0, NULL);
	if (i)
	{
		mciGetErrorString(i, c1, 100);
		printf("%s\n", c1);
	}
	else
	{
		mciSendString("play mymusic", NULL, 0, NULL);
	}
}

//播放音乐，从绝对路径播放
void Play()
{
	if (1 == Mci_Flag)                             //1表示调用过MusicOpen函数
	{
		MusicClose();
		Bro_Flag = -1;             //-1表示未打开音乐
		log_record(ERROR_, "LINE:  %d   music open failed", __LINE__ - 2);
	}
	MusicOpen(BroName);
	Readlrc();
	MusicPlay();
	Bro_Flag = 1;                                  //1表示当前音乐状态是播放
	system("cls");
}

//音乐的播放/暂停
void MusicPauseBroadcast(void)
{
	if (Bro_Flag == 1)             //1表示当前音乐状态是播放
	{
		MusicPause();
		log_record(INFO, "LINE:  %d   music pause", __LINE__ - 1);
		Bro_Flag = 0;
	}
	else if (Bro_Flag == 0)        //0表示当前音乐状态是暂停
	{
		MusicPlay();
		log_record(INFO, "LINE:  %d   music play", __LINE__ - 1);
		Bro_Flag = 1;
	}
	else
	{
		printf("调用函数失败，可能未打开音乐文件\n");
		log_record(ERROR_, "LINE:  %d   func call failed", __LINE__ - 1);
		return;
	}
}




//双缓冲区初始化
void DoubleBuffInitial()
{
	//创建新的控制台缓冲区
	hOutBuf = CreateConsoleScreenBuffer(
		GENERIC_WRITE,           //定义进程可以往缓冲区写数据
		FILE_SHARE_WRITE,        //定义缓冲区可共享写权限
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	hOutput = CreateConsoleScreenBuffer(
		GENERIC_WRITE,           //定义进程可以往缓冲区写数据
		FILE_SHARE_WRITE,        //定义缓冲区可共享写权限
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	hOut[0] = hOutBuf;
	hOut[1] = hOutput;

	//隐藏两个缓冲区的光标
	CONSOLE_CURSOR_INFO cursor;
	cursor.bVisible = FALSE;
	cursor.dwSize = sizeof(cursor);

	SetConsoleCursorInfo(hOutput, &cursor);
	SetConsoleCursorInfo(hOutBuf, &cursor);
}

//双缓冲显示播放界面
void DoubleBuffSet(int music_timeing_ms, int music_alltime, int music_timeing, char* PreSong, char* NextSong)
{
	if (LyricShowFlag)
	{
		memset(PrintfBuff, 0, sizeof PrintfBuff);
		for (int i = 0; i < strlen(BroName) - 4; i++)
		{
			PrintfBuff[i] = BroName[i];

		}
		coord.X = 60;
		coord.Y = 1;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);

		for (p = 1; music_timeing_ms >= LRC[p].time; p++);

		LyricFlag = --p;


		memset(PrintfBuff, 0, sizeof PrintfBuff);
		if (LyricFlag >= 5)
		{
			strcpy(PrintfBuff, LRC[LyricFlag - 4].lyric);
		}
		coord.Y += 2;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);


		memset(PrintfBuff, 0, sizeof PrintfBuff);
		if (LyricFlag >= 4)
		{
			strcpy(PrintfBuff, LRC[LyricFlag - 3].lyric);
		}
		coord.Y += 2;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);

		memset(PrintfBuff, 0, sizeof PrintfBuff);
		if (LyricFlag >= 3)
		{
			strcpy(PrintfBuff, LRC[LyricFlag - 2].lyric);
		}
		coord.Y += 2;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);



		memset(PrintfBuff, 0, sizeof PrintfBuff);
		if (LyricFlag >= 2)
		{
			strcpy(PrintfBuff, LRC[LyricFlag - 1].lyric);
		}
		coord.Y += 2;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);



		memset(PrintfBuff, 0, sizeof PrintfBuff);
		strcpy(PrintfBuff, LRC[LyricFlag].lyric);
		coord.Y += 2;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);

		strcpy(PrintfBuff, "★");
		coord.X = 58;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, 2, coord, &bytes);
		coord.X = 60;



		memset(PrintfBuff, 0, sizeof PrintfBuff);
		if (LyricFlag + 1 <= LinesNumber)
		{
			strcpy(PrintfBuff, LRC[LyricFlag + 1].lyric);
		}
		coord.Y += 2;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);


		memset(PrintfBuff, 0, sizeof PrintfBuff);
		if (LyricFlag + 2 <= LinesNumber)
		{
			strcpy(PrintfBuff, LRC[LyricFlag + 2].lyric);
		}
		coord.Y += 2;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);


		memset(PrintfBuff, 0, sizeof PrintfBuff);
		if (LyricFlag + 3 <= LinesNumber)
		{
			strcpy(PrintfBuff, LRC[LyricFlag + 3].lyric);
		}
		coord.Y += 2;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);


		memset(PrintfBuff, 0, sizeof PrintfBuff);
		if (LyricFlag + 4 <= LinesNumber)
		{
			strcpy(PrintfBuff, LRC[LyricFlag + 4].lyric);
		}
		coord.Y += 2;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);


	}
	else {
		for (coord.Y = 1; coord.Y < 22; coord.Y += 2)
		{
			memset(PrintfBuff, 0, sizeof PrintfBuff);
			WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);
		}
		strcpy(PrintfBuff, "★");
		coord.X = 58;
		coord.Y = 10;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, 2, coord, &bytes);
		memset(PrintfBuff, 0, sizeof PrintfBuff);
		strcpy(PrintfBuff, "未找到歌词文件");
		coord.X = 60;
		WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);

	}

	char progressline[101] = { NULL };
	strcpy(progressline, Getprogress(music_alltime, music_timeing_ms));
	coord.X = 18;
	coord.Y = 23;
	WriteConsoleOutputCharacterA(hSet, progressline, 100, coord, &bytes);

	memset(PrintfBuff, 0, sizeof PrintfBuff);
	sprintf(PrintfBuff, "Time:%d:%02d-%d:%02d", music_timeing / 60, music_timeing % 60, music_alltime / 60, music_alltime % 60); // 显示当前时间和总时间，歌曲名，当前音量和模式
	coord.X = 0;
	coord.Y = 23;
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, 15, coord, &bytes);

	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcat(PrintfBuff, "当前音量：        ");
	char volumeline[13] = { NULL };
	strcpy(volumeline, Getvolumeline(MusicGetVolume()));
	strcat(PrintfBuff, volumeline);
	coord.X = 0;
	coord.Y = 25;
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);


	coord.X = 0;
	coord.Y = 1;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcpy(PrintfBuff, "功能:");
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);

	coord.X = 5;

	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcpy(PrintfBuff, "1暂停\\播放");
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);


	coord.Y += 2;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	sprintf(PrintfBuff, "2上一首（%s)", PreSong);
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);


	coord.Y += 2;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	sprintf(PrintfBuff, "3下一首（%s)", NextSong);
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);


	coord.Y += 2;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcpy(PrintfBuff, "4停止");
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);


	coord.Y += 2;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcpy(PrintfBuff, "5添加到我的歌单");
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);


	coord.Y += 2;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcpy(PrintfBuff, "6增加音量");
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);


	coord.Y += 2;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcpy(PrintfBuff, "7减小音量");
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);



	coord.Y += 2;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcpy(PrintfBuff, "8快进5s");
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);


	coord.Y += 2;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcpy(PrintfBuff, "9快退5s");
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);



	coord.Y += 2;
	memset(PrintfBuff, 0, sizeof PrintfBuff);
	strcpy(PrintfBuff, "0返回主菜单");
	WriteConsoleOutputCharacterA(hSet, PrintfBuff, strlen(PrintfBuff), coord, &bytes);


	memset(PrintfBuff, 0, sizeof PrintfBuff);
	//sprintf(PrintfBuff, "请输入命令指令（注意命令指令是否有效）");
	coord.X = 1;
	coord.Y = 27;
	//WriteConsoleOutputCharacterA(hSet, PrintfBuff, LENGTH, coord, &bytes);
	SetConsoleActiveScreenBuffer(hSet);



	BuffSwitch = !BuffSwitch;
	hSet = hOut[BuffSwitch];
}



//设置光标位置
static void  SetPos(int  x, int  y)
{

	COORD  point = { x ,  y };                   //光标要设置的位置x,y

	SetConsoleCursorPosition(handle_out, point); //设置光标位置
}

//显示音符
void note(int x, int y)
{
	SetPos(x, y++); printf("    ******************************");
	SetPos(x, y++); printf("    ******************************");
	SetPos(x, y++); printf("    ***                        ***");
	SetPos(x, y++); printf("    ***                        ***");
	SetPos(x, y++); printf("    ******************************");
	SetPos(x, y++); printf("    ******************************");
	SetPos(x, y++); printf("    ***                        ***");
	SetPos(x, y++); printf("    ***                        ***");
	SetPos(x, y++); printf("    ***                        ***");
	SetPos(x, y++); printf("    ***                        ***");
	SetPos(x, y++); printf("    ***                        ***");
	SetPos(x, y++); printf("    ***                        ***");
	SetPos(x, y++); printf("    ***                        ***");
	SetPos(x, y++); printf("   ****                        ***");
	SetPos(x, y++); printf("  *****                        ***");
	SetPos(x, y++); printf(" ******                      *****");
	SetPos(x, y++); printf(" ****                       ******");
	SetPos(x, y++); printf("                             ****");
}

//打印主菜单
void PrintMainMenu()
{
	note(45, 1);
	SetPos(0, 20);

	printf("***********************************************************************************************************************\n");
	printf("*                                                                                                                     *\n");
	printf("*                                                          主菜单                                                     *\n");
	printf("*                                                                                                                     *\n");
	printf("*                                                        1.所有歌曲                                                   *\n");
	printf("*                                                                                                                     *\n");
	printf("*                                                        2.我的歌单                                                   *\n");
	printf("*                                                                                                                     *\n");
	printf("*                                                        3.正在播放                                                   *\n");
	printf("*                                                                                                                     *\n");
	printf("*                                                        4.播放模式                                                   *\n");
	printf("*                                                                                                                     *\n");
	printf("*                                                        0.退出                                                       *\n");
	printf("*                                                                                                                     *\n");
	printf("***********************************************************************************************************************");
	printf("\n");
}

//进入曲库
int AllMusic(void)
{
	system("cls");
	ListFlag = 0;
	note(45, 1);

	SetPos(0, 20);

	printf("***********************************************************************************************************************\n");

	for (int i = 1; i <= MusicNumber * 2 + 3; i++)
		printf("*                                                                                                                     *\n");

	printf("***********************************************************************************************************************\n");

	SetPos(58, 22);
	printf("所有歌曲");
	SetPos(57, 24);
	int m = 24;

	for (int i = 1; i <= MusicNumber; i++)
	{
		printf("%d ", i);
		char temps[100] = { NULL };
		strncpy(temps, alls[i], strlen(alls[i]) - 4);
		printf("%s", temps);
		m += 2;
		SetPos(57, m);
	}
	log_record(INFO, "LINE:  %d   print all songs", __LINE__ - 2);

	printf("\n1选择播放歌曲 2返回主菜单\n");

	int tempFlag = -1;
	do
	{
		printf("请输入命令指令（注意命令指令是否有效）\n");
		while (1 != scanf("%d", &tempFlag))
		{
			printf("输入命令格式错误，请重新输入\n");
			log_record(ERROR_, "LINE:  %d   input format error", __LINE__ - 3);
			while (getchar() != '\n');                 //读取所有缓冲区错误命令格式，包括回车
		}
	} while (tempFlag < 1 || tempFlag > 2);

	if (1 == tempFlag)                                 //选择播放歌曲，从绝对id播放
	{
		printf("请输入歌曲id\n");
		scanf("%d", &IDInput);
		strcpy(BroName, alls[IDInput]);
		Play();
		log_record(INFO, "LINE:  %d   play music named  %s  ", __LINE__ - 1, BroName);
		log_record(INFO, "LINE:  %d   read lyric named  %s  ", __LINE__ - 2, BroName);
		return IDInput;
	}
	else return -1;
}

//我的歌单
int MyMusic(void)
{
	SetPos(58, 22);
	printf("我的歌单");
	SetPos(57, 24);
	printf("1选择歌单                 ");
	SetPos(57, 26);
	printf("2创建歌单                   ");
	SetPos(57, 28);
	printf("3删除歌单          ");
	SetPos(57, 30);
	printf("0返回主菜单");
	SetPos(57, 32);
	printf("          ");
	printf("\n\n\n");
	int temp = -1;
	do
	{
		printf("请输入命令指令（注意命令指令是否有效）\n");
		while (1 != scanf("%d", &temp))
		{
			printf("输入命令格式错误，请重新输入\n");
			log_record(ERROR_, "input format error", __LINE__ - 3);
			while (getchar() != '\n');                 //读取所有缓冲区错误命令格式，包括回车
		}
	} while (temp < 0 || temp > 3);

	int m = 22;

	switch (temp)
	{
	case 1:
		system("cls");
		note(45, 1);
		SetPos(0, 20);

		printf("***********************************************************************************************************************\n");

		for (int i = 1; i <= ListNumber * 2 + 3; i++)
			printf("*                                                                                                                     *\n");

		printf("***********************************************************************************************************************\n");

		SetPos(57, 22);

		for (int i = 0; i <= ListNumber; i++)
		{
			if (!i)
			{
				SetPos(58, 22);
				printf("歌单名称");  //
				SetPos(57, 24);
				m += 2;
				continue;
			}
			printf("%d ", i);
			char templ[100] = { NULL };
			strncpy(templ, listname[i], strlen(listname[i]) - 4);
			printf("%s", templ);

			m += 2;
			SetPos(57, m);

		}
		m += 2;
		SetPos(0, m);
		if (!ListNumber) printf("未创建歌单！\n");
		else
		{
			printf("请输入歌单编号\n");
			scanf("%d", &ListFlag);
			log_record(INFO, "LINE:  %d   open  %d  list sucessfully", __LINE__ - 1, ListFlag);
		}
		break;
	case 2:
		printf("请输入歌单名称\n");
		scanf("%s", listname[++ListNumber]);
		log_record(INFO, "LINE:  %d   create new list named %s", __LINE__ - 1, listname[ListNumber]);
		strcat(listname[ListNumber], ".txt");
		SaveList(ListNumber);
		log_record(INFO, "LINE:  %d   save list numbered %d  and  named %s sucessfully", __LINE__ - 1, ListNumber, listname[ListNumber]);
		printf("创建成功，即将返回主菜单");
		Sleep(2000);
		return -1;
		break;
	case 3:
		system("cls");
		note(45, 1);
		SetPos(0, 20);

		printf("***********************************************************************************************************************\n");

		for (int i = 1; i <= ListNumber * 2 + 3; i++)
			printf("*                                                                                                                     *\n");

		printf("***********************************************************************************************************************\n");

		SetPos(57, 22);

		for (int i = 0; i <= ListNumber; i++)
		{
			if (!i)
			{
				SetPos(58, 22);
				printf("歌单名称");  //
				SetPos(57, 24);
				m += 2;
				continue;
			}
			printf("%d ", i);
			char templ[100] = { NULL };
			strncpy(templ, listname[i], strlen(listname[i]) - 4);
			printf("%s", templ);

			m += 2;
			SetPos(57, m);

		}
		m += 2;
		SetPos(0, m);
		printf("请输入待删除的歌单编号：\n");
		int deletelist;
		scanf("%d", &deletelist);

		if (!DeleteList(deletelist))
		{
			log_record(INFO, "LINE:  %d   list %d had been removed!", __LINE__ - 2, deletelist);

			printf("删除歌单成功！\n");
			printf("即将返回菜单\n");
			Sleep(1000);
		}
		else
		{
			log_record(ERROR_, "LINE:  %d   list %d removed failed!", __LINE__ - 10, deletelist);

			printf("删除歌单失败！\n");
			printf("即将返回菜单\n");
			Sleep(1000);
		}
		return -1;
		break;
	case 0:
		return -1;
	}

	if (!ListNumber) return -1;
MARK2:	PrintList(ListFlag);
	log_record(INFO, "LINE:  %d   print  %d  list named %s", __LINE__ - 1, ListFlag, listname[ListFlag]);

	printf("1选择播放歌曲 2从歌单删除歌曲 0返回主菜单\n");
	int tempFlag = -1;
	do
	{
		printf("请输入命令指令（注意命令指令是否有效）\n");
		while (1 != scanf("%d", &tempFlag))
		{
			printf("输入命令格式错误，请重新输入\n");
			log_record(ERROR_, "LINE:  %d   input format error", __LINE__ - 3);
			while (getchar() != '\n');                 //读取所有缓冲区错误命令格式，包括回车
		}
	} while (tempFlag < 0 || tempFlag > 2);


	if (1 == tempFlag)
	{
		if (!ListSize[ListFlag])
		{
			printf("歌单为空，即将返回主菜单");
			Sleep(1000);
			return -1;
		}

		do {
			printf("请输入歌曲id（注意歌曲id是否有效）\n");
			while (1 != scanf("%d", &IDInput))
			{
				printf("输入歌曲id错误，请重新输入\n");
				log_record(ERROR_, "LINE:  %d   input format error", __LINE__ - 3);
				while (getchar() != '\n');                 //读取所有缓冲区错误命令格式，包括回车
			}
		} while (IDInput < 1 || IDInput > ListSize[ListFlag]);              //用户输入合法

		strcpy(BroName, GetName(IDInput, ListFlag));
		Play();
		log_record(INFO, "LINE:  %d   play music named  %s  ", __LINE__ - 1, BroName);
		log_record(INFO, "LINE:  %d   read lyric named  %s  ", __LINE__ - 2, BroName);

		return IDInput;
	}

	else if (2 == tempFlag)
	{
		int MusicNum = -1;
		do {
			printf("请输入待删除的歌曲id（注意歌曲id是否有效）\n");
			while (1 != scanf("%d", &MusicNum))
			{
				printf("输入歌曲id错误，请重新输入\n");
				log_record(ERROR_, "LINE:  %d   input format error", __LINE__ - 3);
				while (getchar() != '\n');                 //读取所有缓冲区错误命令格式，包括回车
			}
		} while (MusicNum < 1 || MusicNum > ListSize[ListFlag]);                               // 默认用户输入合法
		deleteone(MusicNum, ListFlag);
		printf("删除成功\n");
		system("cls");
		SaveList(ListFlag);
		goto MARK2;
		return -1;

	}
	else return -1;

}

//显示播放界面功能
void PrintDisPlayMenu(char* PreSong, char* NextSong)
{
	SetPos(0, 1), printf("功能:");

	SetPos(5, 1), printf("1暂停\\播放");

	SetPos(5, 3), printf("2上一首（%s)", PreSong);

	SetPos(5, 5), printf("3下一首（%s)", NextSong);

	SetPos(5, 7), printf("4停止");

	SetPos(5, 9), printf("5添加到我的歌单");

	SetPos(5, 11), printf("6增加音量");

	SetPos(5, 13), printf("7减小音量");

	SetPos(5, 15), printf("8快进5s");

	SetPos(5, 17), printf("9快退5s");

	SetPos(5, 19), printf("0返回主菜单");

	SetPos(5, 21), printf("请输入命令指令（注意命令指令是否有效）");

}

//播放界面
void Display()
{

	int music_alltime = 0;
	char time_all[100];
	char time_cur[100];


	mciSendString("status mymusic length", time_all, 100, NULL);
	music_alltime = atoi(time_all);         //int总时 6位数
	music_alltime /= 1000;                  //多少秒
	while (!kbhit())
	{
		mciSendString("status mymusic position", time_cur, 100, NULL);
		int music_timeing = atoi(time_cur);
		int music_timeing_ms = music_timeing;
		music_timeing /= 1000;
		//上一首歌歌名和下一首歌歌名
		char PreSong[100] = { NULL }, NextSong[100] = { NULL };

		strncpy(PreSong, GetPrev(), strlen(GetPrev()) - 4);
		strncpy(NextSong, GetNext(), strlen(GetNext()) - 4);
		DoubleBuffSet(music_timeing_ms, music_alltime, music_timeing, PreSong, NextSong);

		Sleep(50);  // 延迟50毫秒，让CPU休息一下

		//结合播放模式和时长
		if (music_timeing == music_alltime - 2)
		{
			char CtrlCmd[CTRLCMD_SIZE];
			char pathindex[100] = "C://CloudMusic//";
			switch (ModeFlag)       //在当前歌单操作
			{
			case 1://顺序播放
				strcpy(BroName, NextSong);
				mciSendString("close mymusic", NULL, 0, NULL);
				strcat(pathindex, BroName);
				sprintf(CtrlCmd, "open \"%s\" alias mymusic", pathindex);
				mciSendString(CtrlCmd, NULL, 0, NULL);
				mciSendString("setaudio mymusic volume to 500", NULL, 0, NULL);
				mciSendString("play mymusic", NULL, 0, NULL);
				mciSendString("status mymusic length", time_all, 100, NULL);

				if (IDInput == ListSize[ListFlag]) IDInput = 1;
				else IDInput++;

				Readlrc();
				break;
			case 2://单曲循环
				mciSendString("seek mymusic to 0", NULL, 0, NULL);
				mciSendString("play mymusic", NULL, 0, NULL);
				break;
			case 3://随机播放
				IDInput = rand() % ListSize[ListFlag] + 1;
				strcpy(BroName, GetName(IDInput, ListFlag));
				mciSendString("close mymusic", NULL, 0, NULL);
				strcat(pathindex, BroName);
				sprintf(CtrlCmd, "open \"%s\" alias mymusic", pathindex);
				mciSendString(CtrlCmd, NULL, 0, NULL);
				mciSendString("setaudio mymusic volume to 500", NULL, 0, NULL);
				mciSendString("play mymusic", NULL, 0, NULL);
				Readlrc();
				mciSendString("status mymusic length", time_all, 100, NULL);

				break;
			}
		}

	}


	SetConsoleActiveScreenBuffer(handle_out);               //切回系统默认缓冲区

}





char* Getprogress(int music_alltime, int music_timeing_ms) //all的单位是秒，ms单位是毫秒
{
	char proline[101] = { NULL };
	int i = 0;
	for (; i < 100; i++)
	{
		if (music_timeing_ms >= music_alltime * 10 * i) proline[i] = '-';
		else break;
	}

	proline[i - 1] = '>';

	for (; i < 100; i++) proline[i] = '-';

	return proline;

}

char* Getvolumeline(int vol)
{
	char volline[13] = { NULL };
	int v = (vol + 20) / 100;
	int i = 0;

	for (; i < v; i++)  volline[i] = '-';
	for (; i < 10; i++) volline[i] = ' ';
	volline[10] = volline[11] = '|';

	volline[12] = '\0';

	return volline;

}

bool exist(const char* path)
{
	char pathindex[100] = { NULL };
	strcpy(pathindex, _PATH);

	FILE* fp;
	strcat(pathindex, path);
	if ((fp = fopen(pathindex, "r")) == NULL)
		return false;

	fclose(fp);
	return true;
}

void addone(const char* name, int ListFlag)
{
	LinkNode node = (LinkNode)malloc(sizeof(MusicNode));
	strcpy(node->name, name);
	node->next = NULL;

	if (head[ListFlag] == NULL)
	{
		node->ReId = 1;
		head[ListFlag] = node;
	}
	else
	{
		LinkNode temp = head[ListFlag];
		while (temp->next != NULL) temp = temp->next;

		node->ReId = temp->ReId + 1;
		temp->next = node;
	}

	ListSize[ListFlag] = node->ReId;

}

void initialalls()
{
	char tempindex[100] = { NULL };
	strcpy(tempindex, _PATH);
	strcat(tempindex, "*.mp3");

	const char* SreachAddr = tempindex;
	long Handle;
	struct _finddata_t FileInfo;
	Handle = _findfirst(SreachAddr, &FileInfo);
	if (-1 == Handle)
		return;

	addalls(FileInfo.name);
	while (!_findnext(Handle, &FileInfo)) addalls(FileInfo.name);

	_findclose(Handle);
}

char* GetName(int ReId, int ListFlag)
{
	if (!ListFlag) return alls[ReId];

	if (ReId == 1) return head[ListFlag]->name;

	LinkNode temp = head[ListFlag];

	while (temp->ReId < ReId) temp = temp->next;

	return temp->name;
}

void PrintList(int ListFlag)
{
	system("cls");

	note(45, 1);

	SetPos(0, 20);

	printf("***********************************************************************************************************************\n");

	for (int i = 1; i <= ListSize[ListFlag] * 2 + 3; i++)
		printf("*                                                                                                                     *\n");

	printf("***********************************************************************************************************************\n");

	SetPos(57, 22);
	int m = 22;

	LinkNode temp = head[ListFlag];



	for (int i = 0; i <= ListSize[ListFlag]; i++)
	{
		if (!i)
		{
			SetPos(58, 22);
			char templ[100] = { NULL };
			strncpy(templ, listname[ListFlag], strlen(listname[ListFlag]) - 4);
			printf("%s", templ);
			SetPos(57, 24);
			m += 2;
			continue;
		}
		char temps[100] = { NULL };
		strncpy(temps, temp->name, strlen(temp->name) - 4);
		printf("%d  %s", i, temps);
		m += 2;
		SetPos(57, m);

		temp = temp->next;
	}
	//printf("%s\n", listname[ListFlag]);

	//歌单为空

	puts("");
}

void SaveList(int ListFlag)
{
	char pathindex[1000] = { NULL };
	strcpy(pathindex, _PATH);

	strcat(pathindex, listname[ListFlag]);

	FILE* fp;
	if ((fp = fopen(pathindex, "w+")) == NULL)
	{
		printf("cannot open file!\n");
		return;
	}

	LinkNode temp = head[ListFlag];
	while (temp != NULL)
	{
		//if (fprintf(fp, "%s\n", temp->name) != strlen(temp->name)) printf("Save ERROR!\n"); 返回值 = strlen(temp->name)+1？
		fprintf(fp, "%s\n", temp->name);
		temp = temp->next;
	}

	printf("更新歌单完毕！\n");

	fclose(fp);
}

void addlist(int n, const char* txtname)
{
	strcpy(listname[n], txtname);

	FILE* fp;

	char compath[100] = { NULL };
	strcpy(compath, _PATH);

	strcat(compath, txtname);
	if ((fp = fopen(compath, "r")) == NULL)
	{
		printf("cannot open file!\n");
		return;
	}

	printf("读取%30s文件成功\n", txtname);
	char tempname[100] = { NULL };
	while (fgets(tempname, 100, fp) != NULL)
	{
		int len = strlen(tempname);
		tempname[len - 1] = '\0'; //去掉换行符
		if (exist(tempname)) addone(tempname, n);
	}

	fclose(fp);
}

void initiallist()
{
	char tempindex[100] = { NULL };
	strcpy(tempindex, _PATH);
	strcat(tempindex, "*.txt");

	const char* SreachAddr = tempindex;
	long Handle;
	struct _finddata_t FileInfo;
	Handle = _findfirst(SreachAddr, &FileInfo);
	if (-1 == Handle)
		return;

	addlist(++ListNumber, FileInfo.name); //FileInfo.name为带后缀的相对路径名
	while (!_findnext(Handle, &FileInfo)) addlist(++ListNumber, FileInfo.name);

	_findclose(Handle);
}

void deleteone(int n, int ListFlag)
{
	LinkNode temp = head[ListFlag];

	if (n <= 0)
	{
		printf("请重新输入歌曲序号！\n");
		return;
	}
	if (!temp)
	{
		char templ[100] = { NULL };
		strncpy(templ, listname[ListFlag], strlen(listname[ListFlag]) - 4);
		printf("%s没有歌曲！\n", templ);
		return;
	}
	if (n == 1)
	{
		head[ListFlag] = head[ListFlag]->next;
		while (temp)
		{
			temp->ReId--;
			temp = temp->next;
		}

		ListSize[ListFlag]--;
		return;
	}
	while ((temp->next)->ReId != n && (temp->next)->next != NULL) temp = temp->next;

	if ((temp->next)->ReId < n)
	{
		char templ[100] = { NULL };
		strncpy(templ, listname[ListFlag], strlen(listname[ListFlag]) - 4);
		printf("%s没有该歌曲！\n", templ);
		return;
	}
	temp->next = (temp->next)->next;
	temp = temp->next;

	while (temp)
	{
		temp->ReId--;
		temp = temp->next;
	}

	ListSize[ListFlag]--;
}

char* GetPrev()
{
	if (!ListFlag)
	{
		if (IDInput == 1) return  alls[MusicNumber];
		else return alls[IDInput - 1];
	}
	LinkNode temp = head[ListFlag];

	if (IDInput == 1)  // 包括了歌单仅有一首歌的情况
	{
		while (temp->next != NULL) temp = temp->next;
		return temp->name;
	}

	while (temp->ReId != IDInput - 1) temp = temp->next;

	return  temp->name;
}

char* GetNext()
{
	if (!ListFlag)
	{
		if (IDInput == MusicNumber) return  alls[1];
		else return alls[IDInput + 1];
	}

	if (IDInput == ListSize[ListFlag]) return head[ListFlag]->name;

	LinkNode temp = head[ListFlag];

	while (temp->ReId != IDInput + 1) temp = temp->next;

	return temp->name;

}

void Readlrc()
{
	LyricShowFlag = 0;

	char* tlyric[100] = { NULL };

	FILE* fp;
	char path[100] = "C://CloudMusic//";
	char pathtemp[100] = { NULL };

	strncpy(pathtemp, BroName, strlen(BroName) - 3);
	strcat(path, pathtemp);
	strcat(path, "lrc");

	if ((fp = fopen(path, "r")) == NULL)
	{
		log_record(WARNING, "LINE:  %d   lyric named  %s  read failed", __LINE__ - 2, BroName);
		return;
	}

	LyricShowFlag = 1;
	fseek(fp, 0, 2);
	int size = ftell(fp);
	char* str = (char*)calloc(1, size + 1);
	rewind(fp);
	fread(str, 1, size, fp);

	LinesNumber = 1;

	tlyric[LinesNumber] = strtok(str, "\r\n");
	while (tlyric[LinesNumber] != NULL)  tlyric[++LinesNumber] = strtok(NULL, "\r\n");
	LinesNumber--;

	fclose(fp);

	for (int i = 5; i <= LinesNumber; i++)
	{
		char s[100] = { NULL };
		int mins[2], secs[4];
		int min, sec;
		strncpy(s, tlyric[i] + 10, strlen(tlyric[i]) - 10);
		s[strlen(tlyric[i]) - 10] = '\0';
		mins[0] = tlyric[i][1] - '0';
		mins[1] = tlyric[i][2] - '0';
		secs[0] = tlyric[i][4] - '0';
		secs[1] = tlyric[i][5] - '0';
		secs[2] = tlyric[i][7] - '0';
		secs[3] = tlyric[i][8] - '0';
		min = mins[0] * 600000 + mins[1] * 60000;
		sec = secs[0] * 10000 + secs[1] * 1000 + secs[2] * 100 + secs[3] * 10;
		LRC[i - 4].time = min + sec;
		if (s)  strcpy(LRC[i - 4].lyric, s);
	}
}

int DeleteList(int n)// head listname ListSize ListNumber
{
	char deletepath[100] = { NULL };
	strcpy(deletepath, _PATH);
	strcat(deletepath, listname[n]);

	for (int i = n; i < ListNumber; i++)
	{
		head[i] = head[i + 1];
		strcpy(listname[i], listname[i + 1]);
		ListSize[i] = ListSize[i + 1];
	}

	head[ListNumber] = NULL;
	memset(listname[ListNumber], 0, sizeof listname[ListNumber]);
	ListSize[ListNumber] = 0;
	ListNumber--;

	if (remove(deletepath) == NULL) return 0;
	else return -1;

}





