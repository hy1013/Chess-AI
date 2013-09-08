////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 源文件：FenBoard.cpp                                                                                   //
// *******************************************************************************************************//
// 中国象棋通用引擎----简单测试引擎示例，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： wying                                                                                        //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 将fen串转化为棋盘信息                                                                               //
// 2. 将棋盘信息转化为fen串                                                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "FenBoard.h"


static const char PieceChar[14] = { 'k', 'r', 'c', 'h', 'b', 'a', 'p', 'K', 'R', 'C', 'H', 'B', 'A', 'P' };


CFenBoard::CFenBoard(void)
{
	// 设定为初始局面，红先手
	strcpy(FenStr, "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR r - - 0 1");
}

CFenBoard::~CFenBoard(void)
{
}


// 将棋子由字符行转化为数字型
// 缺欠：只能转换大写字符(红色棋子)，若遇黑色棋子，可将其-32，小写变为大写
int CFenBoard::FenToPiece(char fen) 
{
	if( fen>='a' && fen<='z' )
		fen -= 32;

	switch (fen) 
	{
		case 'K':			//King将帅
			return 0;
		case 'R':			//Rook车
			return 1;
		case 'C':			//Cannon炮
			return 2;
		case 'N':			//Knight马
		case 'H':			//Horse
			return 3;
		case 'B':			//Bishop像
		case 'E':			//Elephant
			return 4;
		case 'A':			//Advisor士
		case 'G':			//Guard
			return 5;		
		default:
			return 6;		//Pawn兵卒
	}
}


char* CFenBoard::BoardToFen(const int *Board, int Player, const unsigned int nNonCapNum, const unsigned int nCurrentStep, unsigned int *StepRecords)
{
	int x,y;
	unsigned int m,n,p=0;

	strcpy(FenStr, "");
	//char *FenStr = "";

	// 保存棋盘
	for(y=3; y<13; y++)
	{
		m = 0;												//空格计数器
		for(x=3; x<12; x++)
		{
			n = Board[ (y<<4) | x ];
			if( n )
			{
				if(m > 0)									//插入空格数
					FenStr[p++] = char(m + '0');
				FenStr[p++] = PieceChar[nPieceType[n]];		//保存棋子字符
				m = 0;
			}
			else
				m ++;
		}

		if(m > 0)											//插入空格数
			FenStr[p++] = char(m + '0');		
		FenStr[p++] = '/';									//插入行分隔符
	}

	// 去掉最后一个'/'
	FenStr[--p] = '\0';

	// " 移动方 - - 无杀子半回合数 当前半回合数"
	//strcat(FenStr, Player ? " r " : " b ");
	//strcat(FenStr, itoa(10, FenStr, nNonCapNum));
	//strcat(FenStr, " ");
	//strcat(FenStr, itoa(10, FenStr, nCurrentStep));
	char str[32];
	sprintf(str, " %c - - %u %u", Player?'r':'b', nNonCapNum, nCurrentStep);
	//FenStr += strlen(FenStr);
	strcat(FenStr, str);
	p = (unsigned int)strlen(FenStr);
	
	// Save Moves
	if(nCurrentStep>1)
	{
		strcat(FenStr, " moves");
		p += 6;

		for(m=1; m<nCurrentStep; m++)
		{
			x = (StepRecords[m] & 0xFF00) >> 8;		// 起始位置
			y =  StepRecords[m] & 0xFF;				// 终止位置

			FenStr[p++] = ' ';
			FenStr[p++] = char(x & 0xF) -  3 + 'a';
			FenStr[p++] = 12 - char(x >> 4 ) + '0';
			FenStr[p++] = char(y & 0xF) -  3 + 'a';
			FenStr[p++] = 12 - char(y >> 4 ) + '0';
		}

		// 结束
		FenStr[p] = '\0';
	}

	return FenStr;
}


// 将Fen串转化为棋盘信息：Board[256], Piece[48], Player, nNonCapNum, nCurrentStep
// 注意：为了加快运行速度，此函数未对棋盘信息的合法性作任何检验，所以Fen串必须是合法的。
// 例如：每行棋子数目超过9个、棋子数目错误、棋子位置非法等等。
int CFenBoard::FenToBoard(int *Board, int *Piece, int &Player, unsigned int &nNonCapNum, unsigned int &nCurrentStep, const char *FenStr)
{
	unsigned int m, n;
	int BlkPiece[7] = { 16, 17, 19, 21, 23, 25, 27 };
	int RedPiece[7] = { 32, 33, 35, 37, 39, 41, 43 };

	// 清空棋盘数组和棋子数组	
	for(m=0; m<256; m++)
		Board[m] = 0;
	for(m=0; m<48; m++)
		Piece[m] = 0;

	// 读取棋子位置信息，同时将其转化为棋盘坐标Board[256]和棋子坐标Piece[48]
	int x = 3;
	int y = 3;
	char chess = *FenStr;
	while( chess != ' ')							// 串的分段标记
	{
		if(*FenStr == '/')							// 换行标记
		{
			x = 3;									// 从左开始
			y ++;									// 下一行
			if( y >= 13 )
				break;
		}
		else if(chess >= '1' && chess <= '9')		// 数字表示空格(无棋子)的数目
		{
			n = chess - '0';						// 连续无子的数目
			for(m=0; m<n; m++) 
			{
				if(x >= 12)
					break;
				x++;
			}
		} 
		else if (chess >= 'a' && chess <= 'z')		// 黑色棋子
		{
			m = FenToPiece( chess - 32 );			// 'A' - 'a' = -32, 目的就是将小写字符转化为大写字符
			if(x < 12) 
			{
				n = BlkPiece[m];
				Board[ Piece[n] = (y<<4)|x ] = n;
				BlkPiece[m] ++;
			}
			x++;
		}
		else if(chess >= 'A' && chess <= 'Z')		// 红色棋子
		{
			m = FenToPiece( chess );				// 此函数只能识别大写字符
			if(x < 12) 
			{
				n = RedPiece[m];
				Board[ Piece[n] = (y<<4)|x ] = n;
				RedPiece[m] ++;
			}
			x++;
		}
		
		// Next Char
		chess = *(++FenStr);
		if( chess == '\0' )
			return 0;
	}

	// 读取当前移动方Player: b-黑方， !black = white = red  红方
	if(*(FenStr++) == '\0')
		return 1;
	Player = *(FenStr++) == 'b' ? 0:1;

	// Skip 2 Reserved Keys
	if(*(FenStr++) == '\0')    return 1;		// ' '
	if(*(FenStr++) == '\0')    return 1;      // '-'
	if(*(FenStr++) == '\0')    return 1;      // ' '
	if(*(FenStr++) == '\0')    return 1;      // '-'
	if(*(FenStr++) == '\0')    return 1;      // ' '
	

	// 若界面不传送吃子以前的着法，完全可以
	// 解决棋局，可以使用下面的参数	
	nNonCapNum   = 0;
	nCurrentStep = 1;

	return 1;

}