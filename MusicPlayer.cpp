/*

2021/07/31   14:07





*/


#include "MusicPlayer8.0.h"
#include <stdio.h>
#include <io.h>
#include <Windows.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <mmsystem.h>
#include <time.h>

#pragma comment(lib,"winmm.lib")
COORD size = { 120,60 };



int main()
{
	system("color 0B");
	SetConsoleScreenBufferSize(handle_out, size);

	printf("请输入歌曲库所在的文件根目录：    (文件路径分隔符请采用//， 例如C://CloudMusic//） \n");
	scanf("%s", _PATH);
	//strcpy(_PATH, "C://CloudMusic//");


	initialalls();                                         //读取曲库，扫描所有mp3文件
	ListSize[0] = MusicNumber;
	log_record(INFO, "LINE:  %d   path:  initial all songs in the form of .mp3 suffix", __LINE__ - 2, _PATH);

	printf("正在初始化歌曲库和歌单\n");
	initiallist();                                         //初始化歌单，并纠正错误信息
	log_record(INFO, "LINE:  %d   path:  initial all lists in the form of .txt suffix", __LINE__ - 1, _PATH);

	DoubleBuffInitial();                                   //初始化双缓冲

	for (int i = 1; i <= ListNumber; i++) SaveList(i);     //保存纠正后的歌单文件


	log_record(INFO, "LINE:  %d   initial and update songs stored locally and listfiles", __LINE__ - 2);


	srand((unsigned)time(NULL));                           //产生随机数种子

	int iFlag = 1;                                         //退出程序的标志
	int iSignal = -1;                                      //用户在主菜单输入的指令编号
	while (iFlag)
	{
		system("cls");
		PrintMainMenu();

		do
		{
			printf("请输入命令指令（注意命令指令是否有效）\n");
			while (1 != scanf("%d", &iSignal))
			{
				printf("输入，命令格式错误，请重新输入\n");
				log_record(ERROR_, "LINE:  %d   input format error", __LINE__ - 3);
				while (getchar() != '\n');                 //读取所有缓冲区错误命令格式，包括回车
			}
		} while (iSignal < 0 || iSignal > 4);

		switch (iSignal)
		{
		case 1:                         //所有歌曲
			BackFlag = AllMusic();
			log_record(INFO, "LINE:  %d   move to all songs menu", __LINE__ - 1);
			break;
		case 2:                         //我的歌单 
			BackFlag = MyMusic();
			log_record(INFO, "LINE:  %d   move to my playlist menu", __LINE__ - 1);
			break;
		case 3:                         //正在播放
			if (Mci_Flag == 1)
			{
				BackFlag = 1;
				goto MARK1;
			}
			else {
				printf("当前未播放歌曲，即将返回主菜单\n");
				Sleep(3000);
			}
			BackFlag = -1;
			break;
		case 4:                         //播放模式
			note(45, 1);
			SetPos(0, 20);
			printf("***********************************************************************************************************************\n");
			printf("*                                                                                                                     *\n");
			printf("*                                                          播放模式                                                   *\n");
			printf("*                                                                                                                     *\n");
			printf("*                                                        1.顺序播放                                                   *\n");
			printf("*                                                                                                                     *\n");
			printf("*                                                        2.单曲循环                                                   *\n");
			printf("*                                                                                                                     *\n");
			printf("*                                                        3.随机播放                                                   *\n");
			printf("*                                                                                                                     *\n");
			printf("*                                                        当前播放模式为:  %s                                    *\n", Mode[ModeFlag]);
			printf("*                                                                                                                     *\n");
			printf("*                                                                                                                     *\n");
			printf("*                                                                                                                     *\n");
			printf("***********************************************************************************************************************");
			printf("\n");

			do
			{
				printf("请输入想更改的模式（注意命令指令是否有效）\n");
				while (1 != scanf("%d", &ModeFlag))
				{
					printf("输入，命令格式错误，请重新输入\n");
					log_record(ERROR_, "LINE:  %d   input format error", __LINE__ - 3);
					while (getchar() != '\n');         //读取所有缓冲区错误命令格式，包括回车
				}
			} while (ModeFlag < 0 || ModeFlag > 3);
			//输入ModeFlag之后相当于更改了播放模式

			break;
		case 0:                         //用户选择退出程序
			iFlag = -1;
			log_record(INFO, "LINE:  %d   exit mp3 program sucessfully", __LINE__ - 1);
			exit(0);
			break;
		default:                        //其他特殊情况
			system("cls");
			printf("输入命令错误!重新输入\n");
			break;
		}

		if (BackFlag == -1) log_record(INFO, "LINE:  %d   return to mainmenu", 54);

		while (BackFlag != -1)
		{

		MARK1:Display();                                   //进入播放界面

			system("cls");
			char PreSong[100] = { NULL }, NextSong[100] = { NULL };
			strncpy(PreSong, GetPrev(), strlen(GetPrev()) - 4);
			strncpy(NextSong, GetNext(), strlen(GetNext()) - 4);
			PrintDisPlayMenu(PreSong, NextSong);
			SetPos(5, 23);
			printf("请检查输入\n");

			SetPos(5, 24);

			int fFlag;

			do
			{
				while (1 != scanf("%d", &fFlag))
				{
					printf("输入命令格式错误，请重新输入\n");
					log_record(ERROR_, "LINE:  %d   input format error", __LINE__ - 3);
					while (getchar() != '\n');             //读取所有缓冲区错误命令格式，包括回车
				}
			} while (fFlag < 0 || fFlag > 9);

			switch (fFlag)
			{
			case 0:
				BackFlag = -1;
				break;
			case 1:
				MusicPauseBroadcast();                     //播放暂停
				break;
			case 2:
				BackFlag = IDInput;
				strcpy(BroName, GetPrev());

				if (IDInput != 1) --IDInput;
				else IDInput = ListSize[ListFlag];         //第一首的上一首是最后一首

				Play();
				Readlrc();
				log_record(INFO, "LINE:  %d   play music named  %s  ", __LINE__ - 2, BroName);
				log_record(INFO, "LINE:  %d   read lyric named  %s  ", __LINE__ - 2, BroName);
				Display();
				break;
			case 3:
				BackFlag = IDInput;
				strcpy(BroName, GetNext());

				if (IDInput != ListSize[ListFlag]) ++IDInput;
				else IDInput = 1;                          //最后一首的下一首是第一首

				Play();
				Readlrc();
				log_record(INFO, "LINE:  %d   play music named  %s  ", __LINE__ - 2, BroName);
				log_record(INFO, "LINE:  %d   read lyric named  %s  ", __LINE__ - 2, BroName);
				Display();
				break;
			case 4:                                        //停止播放
				MusicStop();
				MusicClose();
				log_record(INFO, "LINE:  %d   music close", __LINE__ - 2);
				break;
			case 5:                                        //添加到我的歌单
				for (int i = 1; i <= ListNumber; i++)      //显示当前已有歌单
				{
					char templ[100] = { NULL };
					strncpy(templ, listname[i], strlen(listname[i]) - 4);
					printf("%d  %s\n", i, templ);
				}

				printf("输入待添加的歌单编号\n");

				int templist;
				scanf("%d", &templist);
				while (templist == ListFlag)
				{
					printf("歌曲已存在！\n");
					break;
				}

				addone(GetName(IDInput, ListFlag), templist);
				SaveList(templist);                        //添加后立刻更新歌单文件
				printf("即将返回播放界面\n");
				Sleep(1000);
				log_record(INFO, "LINE:  %d   save list numbered %d  and  named %s sucessfully", __LINE__ - 2, templist, listname[templist]);
				break;
			case 6:                                        //增加音量
				MusicSetVolumeUp();
				log_record(INFO, "LINE:  %d   music volume up", __LINE__ - 1);

				break;
			case 7:                                        //减小音量
				MusicSetVolumeDown();
				log_record(INFO, "LINE:  %d   music volume down", __LINE__ - 1);

				break;
			case 8:                                        //快进5s
				MusicFastForward();
				log_record(INFO, "LINE:  %d   music fastforward by 5 seconds", __LINE__ - 1);
				break;
			case 9:                                        //快退5s
				MusicFastBackward();
				log_record(INFO, "LINE:  %d   music fastbackward by 5 seconds", __LINE__ - 1);
				break;
			}

		}
	}

	return 0;
}
