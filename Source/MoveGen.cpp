////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 源文件：MoveGen.cpp                                                                                    //
// *******************************************************************************************************//
// 中国象棋通用引擎----兵河五四，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： 范 德 军                                                                                        //
// 单位： 中国原子能科学研究院                                                                            //
// 邮箱： fan_de_jun@sina.com.cn                                                                          //
//  QQ ： 83021504                                                                                        //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 基础类型CMoveGen, CSearch子类继承之。棋盘、棋子、位行、位列、着法等数据在这个类中被定义。           //
// 2. 通用移动产生器                                                                                      //
// 3. 吃子移动产生器                                                                                      //
// 4. 将军逃避移动产生器                                                                                  //
// 5. 杀手移动合法性检验                                                                                  //
// 6. 将军检测Checked(Player), Checking(1-Player)                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include "MoveGen.h"
#include "PreMove.h"


// 棋盘数组和棋子数组
int Board[256];								// 棋盘数组，表示棋子序号：0～15，无子; 16～31,黑子; 32～47, 红子；
int Piece[48];								// 棋子数组，表示棋盘位置：0, 不在棋盘上; 0x33～0xCC, 对应棋盘位置；	

// 位行与位列棋盘
unsigned int xBitBoard[16];					// 16个位行，产生车炮的横向移动，前12位有效
unsigned int yBitBoard[16];					// 16个位列，产生车炮的纵向移动，前13位有效

// 车炮横向与纵向移动的16位棋盘，只用于杀手移动合法性检验、将军检测和将军逃避   							          
unsigned short xBitRookMove[12][512];		//  12288 Bytes, 车的位行棋盘
unsigned short yBitRookMove[13][1024];		//  26624 Bytes  车的位列棋盘
unsigned short xBitCannonMove[12][512];		//  12288 Bytes  炮的位行棋盘
unsigned short yBitCannonMove[13][1024];	//  26624 Bytes  炮的位列棋盘
								  // Total: //  77824 Bytes =  76K

unsigned short HistoryRecord[65535];		// 历史启发，数组下标为: move = (nSrc<<8)|nDst;

const int xBitMask[256] = 
{
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   1,   2,   4,   8,  16,  32,  64, 128, 256,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

const int yBitMask[256] = 
{
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
	0,   0,   0,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0,   0,   0,   0,
	0,   0,   0,   4,   4,   4,   4,   4,   4,   4,   4,   4,   0,   0,   0,   0,
	0,   0,   0,   8,   8,   8,   8,   8,   8,   8,   8,   8,   0,   0,   0,   0,
	0,   0,   0,  16,  16,  16,  16,  16,  16,  16,  16,  16,   0,   0,   0,   0,
	0,   0,   0,  32,  32,  32,  32,  32,  32,  32,  32,  32,   0,   0,   0,   0,
	0,   0,   0,  64,  64,  64,  64,  64,  64,  64,  64,  64,   0,   0,   0,   0,
	0,   0,   0, 128, 128, 128, 128, 128, 128, 128, 128, 128,   0,   0,   0,   0,
	0,   0,   0, 256, 256, 256, 256, 256, 256, 256, 256, 256,   0,   0,   0,   0,
	0,   0,   0, 512, 512, 512, 512, 512, 512, 512, 512, 512,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

// 将车炮马象士兵
static const int MvvValues[48] = 
{
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  10000, 2000, 2000, 1096, 1096, 1088, 1088, 1040, 1040, 1041, 1041, 1017, 1018, 1020, 1018, 1017,
  10000, 2000, 2000, 1096, 1096, 1088, 1088, 1040, 1040, 1041, 1041, 1017, 1018, 1020, 1018, 1017 
};

// 马腿增量表：有两个功能
// 1. nHorseLegTab[nDst-nSrc+256] != 0				// 说明马走“日”子，是伪合法移动
// 2. nLeg = nSrc + nHorseLegTab[nDst-nSrc+256]		// 马腿的格子
//    Board[nLeg] == 0								// 马腿位置没有棋子，马可以从nSrc移动到nDst
const char nHorseLegTab[512] = {
                               0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,-16,  0,-16,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0, -1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0, -1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0, 16,  0, 16,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0
};


// 移动方向索引：适合近距离移动的棋子
// 0 --- 不能移动到那里
// 1 --- 上下左右，适合将帅和兵卒的移动
// 2 --- 士能够到达
// 3 --- 马能够到达
// 4 --- 象能够到达
const char nDirection[512] = {
                                   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  4,  3,  0,  3,  4,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  3,  2,  1,  2,  3,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  1,  0,  1,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  3,  2,  1,  2,  3,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  4,  3,  0,  3,  4,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0
};


// 着法预产生数组
	// 吃子移动 + 普通移动
static unsigned char KingMoves[256][8];				//   2048 Bytes, 将的移动数组
static unsigned char xRookMoves[12][512][12];		//  73728 Bytes, 车的横向移动
static unsigned char yRookMoves[13][1024][12];		// 159744 Bytes, 车的纵向移动
static unsigned char xCannonMoves[12][512][12];		//  73728 Bytes, 炮的横向移动
static unsigned char yCannonMoves[13][1024][12];	// 159744 Bytes, 炮的纵向移动
static unsigned char KnightMoves[256][12];			//   3072 Bytes, 马的移动数组
static unsigned char BishopMoves[256][8];			//   2048 Bytes, 象的移动数组
static unsigned char GuardMoves[256][8];			//   2048 Bytes, 士的移动数组
static unsigned char PawnMoves[2][256][4];		    //   2048 Bytes, 兵的移动数组：0-黑卒， 1-红兵
										     // Total: 478208 Bytes = 467KB
	// 吃子移动
static unsigned char xRookCapMoves[12][512][4];	//  24576 Bytes, 车的横向移动
static unsigned char yRookCapMoves[13][1024][4];	//  53248 Bytes, 车的纵向移动
static unsigned char xCannonCapMoves[12][512][4];	//  24576 Bytes, 炮的横向移动
static unsigned char yCannonCapMoves[13][1024][4];	//  53248 Bytes, 炮的纵向移动
										     // Total: 155648 Bytes = 152KB

CMoveGen::CMoveGen(void)
{
	// 在整个程序生存过程中，CPreMove只需调用一次
	CPreMove PreMove;
	
	PreMove.InitKingMoves(KingMoves);
	PreMove.InitRookMoves(xRookMoves, yRookMoves, xRookCapMoves, yRookCapMoves);
	PreMove.InitCannonMoves(xCannonMoves, yCannonMoves, xCannonCapMoves, yCannonCapMoves);
	PreMove.InitKnightMoves(KnightMoves);
	PreMove.InitBishopMoves(BishopMoves);
	PreMove.InitGuardMoves(GuardMoves);
	PreMove.InitPawnMoves(PawnMoves);

	// 初始化用于车炮横向与纵向移动的16位棋盘
	PreMove.InitBitRookMove(xBitRookMove, yBitRookMove);
	PreMove.InitBitCannonMove(xBitCannonMove, yBitCannonMove);
}

CMoveGen::~CMoveGen(void)
{
}


// 更新历史记录
// nMode==0  清除历史记录
// nMode!=0  衰减历史记录，HistoryRecord[m] >>= nMode;
void CMoveGen::UpdateHistoryRecord(unsigned int nMode)
{
	unsigned int m;
	unsigned int max_move = 0xCCCC;			// 移动的最大值0xFFFF，0xCCCC后面的移动不会用到

	if( nMode )								// 衰减历史分数
	{
		for(m=0; m<max_move; m++)
			HistoryRecord[m] >>= nMode;
	}
	else									// 清零，用于新的局面
	{
		for(m=0; m<max_move; m++)
			HistoryRecord[m] = 0;
	}
}

// 根据Board[256], 产生所有合法的移动： Player==-1(黑方), Player==+1(红方)
// 移动次序从采用 MVV/LVA(吃子移动) 和 历史启发结合，比单独的历史启发快了10%
int CMoveGen::MoveGenerator(const int Player, CChessMove* pGenMove)
{	
	const unsigned int  k = (1+Player) << 4;	    //k=16,黑棋; k=32,红棋。
	unsigned int  move, nSrc, nDst, x, y, nChess;
	CChessMove* ChessMove = pGenMove;		//移动的计数器
	unsigned char *pMove;
							
	// 产生将帅的移动********************************************************************************************
	nChess = k;
	nSrc = Piece[nChess];								// 将帅存在：nSrc!=0
	{
		pMove = KingMoves[nSrc];
		while( *pMove )
		{
			nDst = *(pMove++);
			if( !Board[nDst] )
			{
				move = (nSrc<<8) | nDst;
				*(ChessMove++) = (HistoryRecord[move]<<16) | move;
			}
		}
	}
	nChess ++;


	// 产生车的移动************************************************************************************************
	for( ; nChess<=k+2; nChess++)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{			
			x = nSrc & 0xF;								// 后4位有效
			y = nSrc >> 4;								// 前4位有效

			//车的横向移动：
			pMove = xRookMoves[x][xBitBoard[y]];
			while( *pMove )
			{
				nDst = (nSrc & 0xF0) | (*(pMove++));	// 0x y|x  前4位=y*16， 后4位=x
				if( !Board[nDst] )
				{
					move = (nSrc<<8) | nDst;
					*(ChessMove++) = (HistoryRecord[move]<<16) | move;
				}
			}

			//车的纵向移动
			pMove = yRookMoves[y][yBitBoard[x]];
			while( *pMove )
			{
				nDst = (*(pMove++)) | x;				// 0x y|x  前4位=y*16， 后4位=x
				if( !Board[nDst] )
				{
					move = (nSrc<<8) | nDst;
					*(ChessMove++) = (HistoryRecord[move]<<16) | move;
				}
			}
		}
	}


	// 产生炮的移动************************************************************************************************
	for( ; nChess<=k+4; nChess++)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{			
			x = nSrc & 0xF;								// 后4位有效
			y = nSrc >> 4;								// 前4位有效

			//炮的横向移动
			pMove = xCannonMoves[x][xBitBoard[y]];
			while( *pMove )
			{
				nDst = (nSrc & 0xF0) | (*(pMove++));	// 0x y|x  前4位=y*16， 后4位=x
				if( !Board[nDst] )
				{
					move = (nSrc<<8) | nDst;
					*(ChessMove++) = (HistoryRecord[move]<<16) | move;
				}
			}

			//炮的纵向移动
			pMove = yCannonMoves[y][yBitBoard[x]];
			while( *pMove )
			{
				nDst = (*(pMove++)) | x;		// 0x y|x  前4位=y*16， 后4位=x	
				if( !Board[nDst] )
				{
					move = (nSrc<<8) | nDst;
					*(ChessMove++) = (HistoryRecord[move]<<16) | move;
				}
			}
		}
	}


	// 产生马的移动******************************************************************************************
	for( ; nChess<=k+6; nChess++)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{
			pMove = KnightMoves[nSrc];
			while( *pMove )
			{
				nDst = *(pMove++);
				if( !Board[nSrc+nHorseLegTab[nDst-nSrc+256]] )
				{					
					if( !Board[nDst] )
					{
						move = (nSrc<<8) | nDst;
						*(ChessMove++) = (HistoryRecord[move]<<16) | move;
					}
				}
			}
		}
	}


	// 产生象的移动******************************************************************************************
	for( ; nChess<=k+8; nChess++)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{
			pMove = BishopMoves[nSrc];
			while( *pMove )
			{
				nDst = *(pMove++);
				if( !Board[(nSrc+nDst)>>1] )				//象眼无子
				{
					if( !Board[nDst] )
					{
						move = (nSrc<<8) | nDst;
						*(ChessMove++) = (HistoryRecord[move]<<16) | move;
					}
				}
			}
		}
	}


	// 产生士的移动******************************************************************************************
	for( ; nChess<=k+10; nChess++)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{
			pMove = GuardMoves[nSrc];
			while( *pMove )
			{
				nDst = *(pMove++);
				if( !Board[nDst] )
				{
					move = (nSrc<<8) | nDst;
					*(ChessMove++) = (HistoryRecord[move]<<16) | move;
				}
			}
		}
	}


	// 产生兵卒的移动******************************************************************************************
	for( ; nChess<=k+15; nChess++)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{
			pMove = PawnMoves[Player][nSrc];
			while( *pMove )
			{
				nDst = *(pMove++);
				if( !Board[nDst] )
				{
					move = (nSrc<<8) | nDst;
					*(ChessMove++) = (HistoryRecord[move]<<16) | move;
				}
			}
		}
	}

	return int(ChessMove-pGenMove);
}


// 产生所有合法的吃子移动
// 经分析，吃子移动占搜索时间的23%。
// 马是最慢的，因为马跳八方；兵卒其次，主要是兵卒数目最多。
int CMoveGen::CapMoveGen(const int Player, CChessMove* pGenMove)
{	
	const unsigned int k = (1+Player) << 4;				// k=16,黑棋; k=32,红棋。
	unsigned int  nSrc, nDst, x, y, nChess, nCaptured;	
	CChessMove  *ChessMove = pGenMove;					// 保存最初的移动指针
	unsigned char *pMove;

	nChess = k+15;

	// 产生兵卒的移动******************************************************************************************
	for( ; nChess>=k+11; nChess--)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{
			pMove = PawnMoves[Player][nSrc];
			while( *pMove )
			{
				nCaptured = Board[ nDst = *(pMove++) ];
				if( (nChess ^ nCaptured) >= 48 )		// 异色棋子
					*(ChessMove++) = ((MvvValues[nCaptured] - 20)>>16) | (nSrc<<8) | nDst;
			}
		}
	}


	// 产生士的移动******************************************************************************************
	for( ; nChess>=k+9; nChess--)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{
			pMove = GuardMoves[nSrc];
			while( *pMove )
			{
				nCaptured = Board[ nDst = *(pMove++) ];
				if( (nChess ^ nCaptured) >= 48 )		// 异色棋子
					*(ChessMove++) = ((MvvValues[nCaptured] - 41)>>16) | (nSrc<<8) | nDst;
			}
		}
	}


	// 产生象的移动******************************************************************************************
	for( ; nChess>=k+7; nChess--)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{
			pMove = BishopMoves[nSrc];
			while( *pMove )
			{
				nCaptured = Board[ nDst = *(pMove++) ];
				if( (nChess ^ nCaptured) >= 48 )		// 异色棋子
				{
					if( !Board[(nSrc+nDst)>>1] )					//象眼无子
						*(ChessMove++) = ((MvvValues[nCaptured] - 40)>>16) | (nSrc<<8) | nDst;
				}
			}
		}
	}


	// 产生马的移动******************************************************************************************
	for( ; nChess>=k+5; nChess--)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{
			pMove = KnightMoves[nSrc];
			while( *pMove )
			{
				nCaptured = Board[ nDst = *(pMove++) ];
				if( (nChess ^ nCaptured) >= 48 )		// 异色棋子
				{
					if( !Board[nSrc+nHorseLegTab[nDst-nSrc+256]] )
						*(ChessMove++) = ((MvvValues[nCaptured] - 88)>>16) | (nSrc<<8) | nDst;
				}
			}
		}
	}


	// 产生炮的移动************************************************************************************************
	for( ; nChess>=k+3; nChess--)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{			
			x = nSrc & 0xF;								// 后4位有效
			y = nSrc >> 4;								// 前4位有效

			//炮的横向移动
			pMove = xCannonCapMoves[x][xBitBoard[y]];
			while( *pMove )
			{
				nCaptured = Board[ nDst = (nSrc & 0xF0) | (*(pMove++)) ];	// 0x y|x  前4位=y*16， 后4位=x
				if( (nChess ^ nCaptured) >= 48 )		// 异色棋子
					*(ChessMove++) = ((MvvValues[nCaptured] - 96)>>16) | (nSrc<<8) | nDst;
			}

			//炮的纵向移动
			pMove = yCannonCapMoves[y][yBitBoard[x]];
			while( *pMove )
			{		
				nCaptured = Board[ nDst = (*(pMove++)) | x ];		// 0x y|x  前4位=y*16， 后4位=x
				if( (nChess ^ nCaptured) >= 48 )		// 异色棋子
					*(ChessMove++) = ((MvvValues[nCaptured] - 96)>>16) | (nSrc<<8) | nDst;
			}
		}
	}


	// 产生车的移动************************************************************************************************
	for( ; nChess>=k+1; nChess--)
	{
		if( nSrc = Piece[nChess] )						// 棋子存在：nSrc!=0
		{			
			x = nSrc & 0xF;								// 后4位有效
			y = nSrc >> 4;								// 前4位有效

			//车的横向移动
			pMove = xRookCapMoves[x][xBitBoard[y]];
			while( *pMove )
			{
				nCaptured = Board[ nDst = (nSrc & 0xF0) | (*(pMove++)) ];	// 0x y|x  前4位=y*16， 后4位=x
				if( (nChess ^ nCaptured) >= 48 )		// 异色棋子
					*(ChessMove++) = ((MvvValues[nCaptured] - 200)>>16) | (nSrc<<8) | nDst;
			}

			//车的纵向移动
			pMove = yRookCapMoves[y][yBitBoard[x]];
			while( *pMove )
			{
				nCaptured = Board[ nDst = (*(pMove++)) | x ];		// 0x y|x  前4位=y*16， 后4位=x
				if( (nChess ^ nCaptured) >= 48 )		// 异色棋子
					*(ChessMove++) = ((MvvValues[nCaptured] - 200)>>16) | (nSrc<<8) | nDst;
			}
		}
	}
                                                                                                                                                                                                                                                                                                                                                     

	// 产生将帅的移动********************************************************************************************
	nSrc = Piece[nChess];								// 棋子存在：nSrc!=0
	{
		pMove = KingMoves[nSrc];
		while( *pMove )
		{
			nCaptured = Board[ nDst = *(pMove++) ];
			if( (nChess ^ nCaptured) >= 48 )		// 异色棋子
				*(ChessMove++) = ((MvvValues[nCaptured] - 1000)>>16) | (nSrc<<8) | nDst;
		}
	}	

	return int(ChessMove-pGenMove);
}


// 判断杀手启发着法的合法性
int CMoveGen::IsLegalKillerMove(int Player, const CChessMove KillerMove)
{	
	int nSrc = (KillerMove & 0xFF00) >> 8;
	int nMovedChs = Board[nSrc];
	if( (nMovedChs >> 4) != Player+1 )			// 若杀手不是本方的棋子，视为非法移动
		return 0;

	int nDst = KillerMove & 0xFF;
	if( (Board[nDst] >> 4) == Player+1 )		// 若杀手为吃子移动，吃同色棋子视为非法
		return 0;

	int x, y;
	switch( nPieceID[nMovedChs] )
	{
		case 1:		// 车
			x = nSrc & 0xF;
			y = nSrc >> 4;
			if( x == (nDst & 0xF) )
				return yBitRookMove[y][yBitBoard[x]] & yBitMask[nDst];		// x相等的纵向移动
			else if( y == (nDst >> 4) )
				return xBitRookMove[x][xBitBoard[y]] & xBitMask[nDst];		// y相等的纵向移动
			break;

		case 2:		// 炮
			x = nSrc & 0xF;
			y = nSrc >> 4;
			if( x == (nDst & 0xF) )
				return yBitCannonMove[y][yBitBoard[x]] & yBitMask[nDst];	// x相等的纵向移动
			else if( y == (nDst >> 4) )
				return xBitCannonMove[x][xBitBoard[y]] & xBitMask[nDst];	// y相等的纵向移动
			break;

		case 3:		// 马
			if( !Board[ nSrc + nHorseLegTab[ nDst-nSrc+256 ] ] )		// 马腿无子
				return 3;
			break;

		case 4:		// 象
			if( !Board[(nSrc+nDst)>>1] )									// 象眼无子
				return 4;
			break;

		default:
			return 1;														// 杀手启发, 将士兵　必然合法
			break;
	}

	return 0;
}

// 使用位行与位列技术实现的将军检测
// 函数一旦遇到将军，立即返回非“0”的数值
// 此函数用于当前移动方是否被将军
// 注意：车炮的位行与位列操作nDst->nSrc与杀手着法合理性检验nSrc->nDst正好相反
int CMoveGen::Checked(int Player)
{
	nCheckCounts ++;
	
	int nKingSq = Piece[(1+Player)<<4];		// 我方将帅的位置
	int x = nKingSq & 0xF;
	int y = nKingSq >> 4;
	int king = (2-Player) << 4 ;				// 对方将帅的序号

	int xBitMove = xBitRookMove[x][xBitBoard[y]];
	int yBitMove = yBitRookMove[y][yBitBoard[x]];
	
	// 双王照面：以讲当车，使用车的位列棋盘
	int nSrc = Piece[king];
	if( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] )
		return nSrc;

	// 车将军
	nSrc = Piece[king+1];
	if( ( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] ) ||
		( y==(nSrc >>  4) && xBitMove & xBitMask[nSrc] ) )
		return nSrc;

	nSrc = Piece[king+2];
	if( ( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] ) ||
		( y==(nSrc >>  4) && xBitMove & xBitMask[nSrc] ) )
		return nSrc;	


	xBitMove = xBitCannonMove[x][xBitBoard[y]];
	yBitMove = yBitCannonMove[y][yBitBoard[x]];

	// 炮将军
	nSrc = Piece[king+3];
	if( ( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] ) ||
		( y==(nSrc >>  4) && xBitMove & xBitMask[nSrc]) )
		return nSrc;

	nSrc = Piece[king+4];
	if( ( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] ) ||
		( y==(nSrc >>  4) && xBitMove & xBitMask[nSrc]) )		
		return nSrc;


	// 被对方的马将军
	nSrc = Piece[king+5];
	x = nHorseLegTab[nKingSq - nSrc + 256];
	if( x && !Board[nSrc + x] )							// 若马走日且马腿无子，马将军
		return nSrc;

	nSrc = Piece[king+6];
	x = nHorseLegTab[nKingSq - nSrc + 256];
	if( x && !Board[nSrc + x] )							// 若马走日且马腿无子，马将军
		return nSrc;


	// 被对方过河的兵卒将军
	if( nPieceType[ Board[Player ? nKingSq-16 : nKingSq+16] ] == 13-7*Player )		// 注意：将帅在中线时，不能有己方的兵卒存在
		return Player ? nKingSq-16 : nKingSq+16;

	if( nPieceID[ Board[nKingSq-1] ]==6 )
		return nKingSq-1;
		
	if( nPieceID[ Board[nKingSq+1] ]==6 )
		return nKingSq+1;

	nNonCheckCounts ++;
	return 0;
}


// 使用位行与位列技术实现的将军检测
// 计算所有能够攻击将帅的棋子checkers，将军逃避函数能够直接使用checkers变量，避免重复计算
// 返回值：checkers!=0，表示将军；checkers==0，表示没有将军
// checkers的后八位表示将军的类型，分别表示：兵0x80 兵0x40 马0x20 马0x10 炮0x08 炮0x04 车0x02 车0x01
// 使用此函数前，需先用Checked()判断我方是否被将军，然后利用此函数计算对方是否被将军
// 注意：因为前面已经使用了Checked()，所以此函数不必要对双王照面再次进行检测
// 注意：车炮的位行与位列操作nDst->nSrc与杀手着法合理性检验nSrc->nDst正好相反，这样设计可以减少计算
int CMoveGen::Checking(int Player)
{
	nCheckCounts ++;
	
	int nKingSq = Piece[(1+Player)<<4];		// 计算将帅的位置
	int x = nKingSq & 0xF;
	int y = nKingSq >> 4;
	int king = (2-Player) << 4 ;			// 将帅
	int checkers = 0;
	int nSrc;

	int xBitMove = xBitRookMove[x][xBitBoard[y]];
	int yBitMove = yBitRookMove[y][yBitBoard[x]];

	// 双王照面：无需检测，不会发生
	//nSrc = Piece[king];
	//if( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] )
	//	checkers |= 0xFF;

	// 车将军：0x01表示车king+1, 0x02表示车king+2
	nSrc = Piece[king+1];
	if( ( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] ) ||
		( y==(nSrc >>  4) && xBitMove & xBitMask[nSrc] ) )
		checkers |= 0x01;

	nSrc = Piece[king+2];
	if( ( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] ) ||
		( y==(nSrc >>  4) && xBitMove & xBitMask[nSrc] ) )
		checkers |= 0x02;


	xBitMove = xBitCannonMove[x][xBitBoard[y]];
	yBitMove = yBitCannonMove[y][yBitBoard[x]];
	
	// 炮将军：0x04表示炮king+3, 0x08表示炮king+4
	nSrc = Piece[king+3];
	if( ( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] ) ||
		( y==(nSrc >>  4) && xBitMove & xBitMask[nSrc]) )
		checkers |= 0x04;

	nSrc = Piece[king+4];
	if( ( x==(nSrc & 0xF) && yBitMove & yBitMask[nSrc] ) ||
		( y==(nSrc >>  4) && xBitMove & xBitMask[nSrc]) )		
		checkers |= 0x08;


	// 马将军：0x10表示马king+5，0x20表示马king+6
	nSrc = Piece[king+5];
	x = nHorseLegTab[nKingSq - nSrc + 256];
	if( x && !Board[nSrc + x] )							// 若马走日且马腿无子，马将军
		checkers |= 0x10;

	nSrc = Piece[king+6];
	x = nHorseLegTab[nKingSq - nSrc + 256];
	if( x && !Board[nSrc + x] )							// 若马走日且马腿无子，马将军
		checkers |= 0x20;


	// 纵向兵卒：0x40表示纵向的兵/卒将军
	if( nPieceType[ Board[Player ? nKingSq-16 : nKingSq+16] ] == 13-7*Player )		// 将帅在中线时，不能有己方的中兵存在
		checkers |= 0x40;

	// 横向兵卒：0x80表示横向的兵/卒将军
	if( nPieceID[ Board[nKingSq-1] ]==6 || nPieceID[ Board[nKingSq+1] ]==6 )		// 查询将帅左右是否有兵/卒存在
		checkers |= 0x80;


	if(!checkers)
		nNonCheckCounts ++;
	return checkers;
}

// 保护判断函数
// 棋子从from->nDst, Player一方是否形成保护
// 试验表明：保护对吃子走法排序起到的作用很小。
int CMoveGen::Protected(int Player, int from, int nDst)
{
	const int king = (2-Player) << 4 ;			// 将帅
	
	//****************************************************************************************************
	// 将
	int nSrc = Piece[king];
	if( nDirection[nDst-nSrc+256]==1 && nCityIndex[nDst] )
		return 1000;

	// 象
	nSrc = Piece[king+7];
	if( nSrc && nSrc!=nDst && nDirection[nDst-nSrc+256]==4 && !Board[(nSrc+nDst)>>1] && (nSrc^nDst)<128 )
		return 40;

	nSrc = Piece[king+8];
	if( nSrc && nSrc!=nDst && nDirection[nDst-nSrc+256]==4 && !Board[(nSrc+nDst)>>1] && (nSrc^nDst)<128 )
		return 40;

	// 士
	nSrc = Piece[king+9];
	if( nSrc && nSrc!=nDst && nDirection[nDst-nSrc+256]==2 && nCityIndex[nDst] )
		return 41;

	nSrc = Piece[king+10];
	if( nSrc && nSrc!=nDst && nDirection[nDst-nSrc+256]==2 && nCityIndex[nDst] )
		return 41;

	// 兵卒
	nSrc = Player ? nDst-16 : nDst+16;
	if( nPieceType[ Board[nSrc] ] == 13-7*Player )		// 注意：将帅在中线时，不能有己方的兵卒存在
		return 20;

	if( (Player && nDst<128) || (!Player && nDst>=128) )
	{
		if( nPieceID[ Board[nDst-1] ]==6 )
			return 17;
			
		if( nPieceID[ Board[nDst+1] ]==6 )
			return 17;
	}

	//*****************************************************************************************************
	// 车、炮、马保护时，有必要清除起点位置from处的棋子，才能够计算准确
	int x = nDst & 0xF;
	int y = nDst >> 4;
	int xBitIndex = xBitBoard[y] ^ xBitMask[from];		// 清除from之位行
	int yBitIndex = yBitBoard[x] ^ yBitMask[from];		// 清除from之位列

	const int m_Piece = Board[from];					// 保存from之棋子
	Piece[m_Piece] = 0;									// 临时清除，以后恢复

	// 车
	nSrc = Piece[king+1];
	if( nSrc && nSrc!=nDst )
	{
		if(	( x==(nSrc & 0xF) && yBitRookMove[y][yBitIndex] & yBitMask[nSrc] ) ||
			( y==(nSrc >>  4) && xBitRookMove[x][xBitIndex] & xBitMask[nSrc] ) )
		{
			Piece[m_Piece] = from;
			return 200;
		}
	}

	nSrc = Piece[king+2];
	if( nSrc && nSrc!=nDst )
	{
		if( ( x==(nSrc & 0xF) && yBitRookMove[y][yBitIndex] & yBitMask[nSrc] ) ||
			( y==(nSrc >> 4) && xBitRookMove[x][xBitIndex] & xBitMask[nSrc] ) )
		{
			Piece[m_Piece] = from;
			return 200;
		}
	}

	// 炮
	nSrc = Piece[king+3];
	if( nSrc && nSrc!=nDst )
	{
		if( ( x==(nSrc & 0xF) && yBitCannonMove[y][yBitIndex] & yBitMask[nSrc] ) ||
			( y==(nSrc >>  4) && xBitCannonMove[x][xBitIndex] & xBitMask[nSrc]) )
		{
			Piece[m_Piece] = from;
			return 96;
		}
	}

	nSrc = Piece[king+4];
	if( nSrc && nSrc!=nDst )
	{
		if( ( x==(nSrc & 0xF) && yBitCannonMove[y][yBitIndex] & yBitMask[nSrc] ) ||
			( y==(nSrc >>  4) && xBitCannonMove[x][xBitIndex] & xBitMask[nSrc]) )		
		{
			Piece[m_Piece] = from;
			return 96;
		}
	}


	// 马
	nSrc = Piece[king+5];
	if( nSrc!=nDst )
	{
		x = nHorseLegTab[nDst - nSrc + 256];
		if( x && (!Board[nSrc + x] || x==from) )			// 若马走日且马腿无子，马将军
		{
			Piece[m_Piece] = from;
			return 88;
		}
	}

	nSrc = Piece[king+6];
	if( nSrc!=nDst )
	{
		x = nHorseLegTab[nDst - nSrc + 256];
		if( x && (!Board[nSrc + x] || x==from) )			// 若马走日且马腿无子，马将军
		{
			Piece[m_Piece] = from;
			return 88;
		}
	}

	// 恢复from处的棋子
	Piece[m_Piece] = from;

	return 0;
}

int CMoveGen::AddLegalMove(const int nChess, const int nSrc, const int nDst, CChessMove *ChessMove)
{
	int x, y, nCaptured;
	
	switch( nPieceID[nChess] )
	{
		case 0:		// 将行上下左右、在九宫中
			if( nDirection[nDst-nSrc+256]==1 && nCityIndex[nDst] )
			{
				nCaptured = Board[nDst];
				if( !nCaptured )
					*(ChessMove++) = (nSrc<<8) | nDst;
				else if( (nChess^nCaptured) >= 48 )
					*(ChessMove++) = (MvvValues[nCaptured]<<16) | (nSrc<<8) | nDst;
				return 1;
			}
			break;

		case 1:		// 车
			x = nSrc & 0xF;
			y = nSrc >> 4;
			if( (x==(nDst & 0xF) && yBitRookMove[y][yBitBoard[x]] & yBitMask[nDst]) ||
				(y==(nDst >>  4) && xBitRookMove[x][xBitBoard[y]] & xBitMask[nDst]) )
			{
				nCaptured = Board[nDst];
				if( !nCaptured )
					*(ChessMove++) = (nSrc<<8) | nDst;
				else if( (nChess^nCaptured) >= 48 )
					*(ChessMove++) = ((MvvValues[nCaptured]+1)<<16) | (nSrc<<8) | nDst;
				return 1;
			}
			break;

		case 2:		// 炮
			x = nSrc & 0xF;
			y = nSrc >> 4;
			if( (x==(nDst & 0xF) && yBitCannonMove[y][yBitBoard[x]] & yBitMask[nDst]) ||
				(y==(nDst >>  4) && xBitCannonMove[x][xBitBoard[y]] & xBitMask[nDst]) )
			{
				nCaptured = Board[nDst];
				if( !nCaptured )
					*(ChessMove++) = (nSrc<<8) | nDst;
				else if( (nChess^nCaptured) >= 48 )
					*(ChessMove++) = ((MvvValues[nCaptured]+3)<<16) | (nSrc<<8) | nDst;
				return 1;
			}
			break;

		case 3:		// 马走日、马腿无子
			x = nHorseLegTab[nDst - nSrc + 256];
			if( x && !Board[nSrc + x] )
			{
				nCaptured = Board[nDst];
				if( !nCaptured )
					*(ChessMove++) = (nSrc<<8) | nDst;
				else if( (nChess^nCaptured) >= 48 )
					*(ChessMove++) = ((MvvValues[nCaptured]+3)<<16) | (nSrc<<8) | nDst;
				return 1;
			}
			break;

		case 4:		// 象走田、象眼无子、象未过河
			if( nDirection[nDst-nSrc+256]==4 && !Board[(nSrc+nDst)>>1] && (nSrc^nDst)<128 )
			{
				nCaptured = Board[nDst];
				if( !nCaptured )
					*(ChessMove++) = (nSrc<<8) | nDst;
				else if( (nChess^nCaptured) >= 48 )
					*(ChessMove++) = ((MvvValues[nCaptured]+5)<<16) | (nSrc<<8) | nDst;
				return 1;
			}
			break;

		case 5:		// 士走斜线、在九宫中
			if( nDirection[nDst-nSrc+256]==2 && nCityIndex[nDst] )
			{
				nCaptured = Board[nDst];
				if( !nCaptured )
					*(ChessMove++) = (nSrc<<8) | nDst;
				else if( (nChess^nCaptured) >= 48 )
					*(ChessMove++) = ((MvvValues[nCaptured]+7)<<16) | (nSrc<<8) | nDst;
				return 1;
			}
			break;

		case 6:		// 兵卒行上下左右、走法合理
			x = nDst-nSrc;						// 当前移动方，Player
			if(  nDirection[x+256]==1 && (
				(nChess<32  && (x==16 || (nSrc>=128 && (x==1 || x==-1)))) ||
				(nChess>=32 && (x==-16 || (nSrc<128 && (x==1 || x==-1)))) ) )
			{
				nCaptured = Board[nDst];
				if( !nCaptured )
					*(ChessMove++) = (nSrc<<8) | nDst;
				else if( (nChess^nCaptured) >= 48 )
					*(ChessMove++) = ((MvvValues[nCaptured]+9)<<16) | (nSrc<<8) | nDst;
				return 1;
			}
			break;

		default:
			break;
	}

	return 0;
}


//********************将军逃避产生器：CheckEvasionGen()***********************************************
// 此函数并非起到完全解将的目的，但能够把将军检测判断的次数大大减少。
// 若完全解将，代码非常复杂，每当移动一颗棋子，都要考虑是否让对方的将车炮马兵等获得自由而构成新的将军。
// 既然是不完全解将，后面一定要加将军检测。这样会比完全解将的运算量少一些，有可能提前得到剪枝。
// Player   表示被将军的一方，即当前移动方
// checkers 将军检测函数Checking(Player)的返回值，表示将军的类型，可以立即知道被哪几颗棋子将军
//          后8位有效，分别表示：横兵、纵兵、马2、马1、炮2、炮1、车2、车1
//          之所以不包含将帅的将军信息，是因为Checked(Player)函数已经作了处理。
int CMoveGen::CheckEvasionGen(const int Player, int checkers, CChessMove* pGenMove)
{
	nCheckEvasions ++;							// 统计解将函数的运行次数

	const int MyKing   = (1+Player) << 4;		// MyKing=16,黑棋; MyKing=32,红棋。
	const int OpKing = (2-Player) << 4;			// 对方王的棋子序号
	const int nKingSq = Piece[MyKing];			// 计算将帅的位置

	int nDir0=0, nDir1=0;						// 将军方向：1－横向；16－纵向
	int nCheckSq0, nCheckSq1;					// 将军棋子的位置
	int nMin0, nMax0, nMin1, nMax1;				// 用于车炮与将帅之间的范围
	int nPaojiazi0, nPaojiazi1;					// 炮架子的位置	
	int nPaojiaziID0, nPaojiaziID1;				// 炮架子的棋子类型

	int nChess, nCaptured, nSrc, nDst, x, y;
	unsigned char *pMove;
	CChessMove *ChessMove = pGenMove;			// 初始化移动的指针

	// 解含炮将军的方法最复杂，针对含炮将军局面作特殊处理，以减少重复计算
	if( checkers & 0x0C )
	{
		// 第一个炮将军的位置
		nCheckSq0 = Piece[ OpKing + (checkers&0x04 ? 3:4) ];

		// 将军方向：1－横向；16－纵向
		nDir0 = (nKingSq&0xF)==(nCheckSq0&0xF) ? 16:1;

		// 炮将之间的范围[nMin0, nMax0)，不包含将帅的位置
		nMin0 = nKingSq>nCheckSq0 ? nCheckSq0 : nKingSq+nDir0;
		nMax0 = nKingSq<nCheckSq0 ? nCheckSq0 : nKingSq-nDir0;		

		// 寻找炮架子的位置：nPaojiazi0
		// 计算炮架子的类型：nPaojiaziID0
		for(nDst=nMin0; nDst<=nMax0; nDst+=nDir0)
		{
			if( Board[nDst] && nDst!=nCheckSq0 )
			{
				nPaojiazi0 = nDst;
				nPaojiaziID0 = Board[nDst];
				break;
			}
		}
		
		// 若炮架子是对方的炮，产生界于双炮之间的移动
		if( nPaojiaziID0==OpKing+3 || nPaojiaziID0==OpKing+4 )
		{
			nMin0 = nPaojiazi0>nCheckSq0 ? nCheckSq0 : nPaojiazi0+nDir0;
			nMax0 = nPaojiazi0<nCheckSq0 ? nCheckSq0 : nPaojiazi0-nDir0;
		}
	}

	// 根据“将军类型”进行解将
	// 采用穷举法，逐个分析每种能够解将的类型
	switch( checkers )
	{
		// 单车将军：杀、挡
		case 0x01:
		case 0x02:
			nCheckSq0 = Piece[ OpKing + (checkers&0x01 ? 1:2) ];			
			nDir0 = (nKingSq&0xF)==(nCheckSq0&0xF) ? 16:1;
			nMin0 = nKingSq>nCheckSq0 ? nCheckSq0 : nKingSq+nDir0;
			nMax0 = nKingSq<nCheckSq0 ? nCheckSq0 : nKingSq-nDir0;

			x = nDir0==1 ? MyKing+10 : MyKing+15;
			for(nDst=nMin0; nDst<=nMax0; nDst+=nDir0)
			{	
				// 车炮马象士兵，杀车或者挡车
				for(nChess=MyKing+1; nChess<=x; nChess++)
				{
					if( (nSrc=Piece[nChess]) != 0 )
						ChessMove += AddLegalMove(nChess, nSrc, nDst, ChessMove);
				}
			}

			break;


		// 单炮将军：杀、挡
		case 0x04:  // 炮1
		case 0x08:  // 炮2
			// 杀炮、阻挡炮将军；暂不产生炮架子的移动
			// 炮架子本身也可以杀炮，但不能产生将军方向的非吃子移动
			x = nDir0==1 ? MyKing+10 : MyKing+15;
			for(nDst=nMin0; nDst<=nMax0; nDst+=nDir0)
			{	
				if( nDst==nPaojiazi0 )							// 杀炮架子无用！
					continue;

				// 车炮马象士兵，杀炮或者挡炮
				for(nChess=MyKing+1; nChess<=x; nChess++)
				{
					nSrc = Piece[nChess];
					if(nSrc && nSrc!=nPaojiazi0)				// 炮架子是我方棋子，暂不产生这种移动
						ChessMove += AddLegalMove(nChess, nSrc, nDst, ChessMove);
				}
			}

			// 产生撤炮架子的移动，形成空头炮，使炮丧失将军能力。
			// 若炮架子是车兵，可以杀将军的炮；炮架子是炮，可以越过对方的炮或者己方的将杀子逃离。
			// 对于车炮兵三个滑块棋子，应该禁止将军方向上的非吃子移动，移动后还是炮架子；
			// 倘若对方炮的背后，存在另一个炮，唯有用炮打吃，可以解将，否则棋子撤离后会形成双炮之势
			if( (nPaojiaziID0-16)>>4==Player )					// 炮架子是己方的棋子
			{
				nSrc = nPaojiazi0;
				nChess = Board[nSrc];
				x = nSrc & 0xF;								// 后4位有效
				y = nSrc >> 4;								// 前4位有效

				switch( nPieceID[Board[nSrc]] )
				{
					case 1:			
						// 车的横向移动：纵向将军时，车可以横向撤离
						//               横向将军时，车可以杀炮
						pMove = xRookMoves[x][xBitBoard[y]];
						while( *pMove )
						{
							nDst = (nSrc & 0xF0) | (*(pMove++));	// 0x y|x  前4位=y*16， 后4位=x
							nCaptured = Board[nDst];

							if( !nCaptured && nDir0==16 )
								*(ChessMove++) = (nSrc<<8) | nDst;
							else if( (nChess^nCaptured)>=48 )
								*(ChessMove++) = (MvvValues[nCaptured]>>16) | (nSrc<<8) | nDst;
						}
						// 车的纵向移动：横向将军时，车可以纵向撤离
						pMove = yRookMoves[y][yBitBoard[x]];
						while( *pMove )
						{
							nDst = (*(pMove++)) | x;				// 0x y|x  前4位=y*16， 后4位=x
							nCaptured = Board[nDst];

							if( !nCaptured && nDir0==1 )
								*(ChessMove++) = (nSrc<<8) | nDst;
							else if( (nChess^nCaptured)>=48 )
								*(ChessMove++) = (MvvValues[nCaptured]>>16) | (nSrc<<8) | nDst;
						}
						break;

					case 2:
						// 炮的横向移动：纵向将军时，炮可以横向撤离；无论纵横将军，炮都可以吃子逃离
						pMove = xCannonMoves[x][xBitBoard[y]];
						while( *pMove )
						{
							nDst = (nSrc & 0xF0) | (*(pMove++));	// 0x y|x  前4位=y*16， 后4位=x
							nCaptured = Board[nDst];

							if( !nCaptured && nDir0==16 )
								*(ChessMove++) = (nSrc<<8) | nDst;
							else if( (nChess^nCaptured) >= 48 )
								*(ChessMove++) = (MvvValues[nCaptured]>>16) | (nSrc<<8) | nDst;
						}

						// 炮的纵向移动：横向将军时，炮可以纵向撤离；无论纵横将军，炮都可以吃子逃离
						pMove = yCannonMoves[y][yBitBoard[x]];
						while( *pMove )
						{
							nDst = (*(pMove++)) | x;		// 0x y|x  前4位=y*16， 后4位=x
							nCaptured = Board[nDst];

							if( !nCaptured && nDir0==1 )
								*(ChessMove++) = (nSrc<<8) | nDst;
							else if( (nChess^nCaptured) >= 48 )
								*(ChessMove++) = (MvvValues[nCaptured]>>16) | (nSrc<<8) | nDst;
						}
						break;

					case 3:		
						// 马：逃离将军方向
						pMove = KnightMoves[nSrc];
						while( *pMove )
						{
							nDst = *(pMove++);			
							nCaptured = Board[nDst];

							if( !Board[nSrc+nHorseLegTab[nDst-nSrc+256]] )
							{
								if( !nCaptured )
									*(ChessMove++) = (nSrc<<8) | nDst;
								else if( (nChess^nCaptured) >= 48 )
									*(ChessMove++) = (MvvValues[nCaptured]>>16) | (nSrc<<8) | nDst;
							}
						}
						break;

					case 4:
						// 象：逃离将军方向
						pMove = BishopMoves[nSrc];
						while( *pMove )
						{
							nDst = *(pMove++);				
							nCaptured = Board[nDst];

							if( !Board[(nSrc+nDst)>>1] )					//象眼无子
							{
								if( !nCaptured )
									*(ChessMove++) = (nSrc<<8) | nDst;
								else if( (nChess^nCaptured) >= 48 )
									*(ChessMove++) = (MvvValues[nCaptured]>>16) | (nSrc<<8) | nDst;
							}
						}
						break;

					case 5:
						// 士：逃离将军方向
						pMove = GuardMoves[nSrc];
						while( *pMove )
						{
							nDst = *(pMove++);
							nCaptured = Board[nDst];

							if( !nCaptured )
								*(ChessMove++) = (nSrc<<8) | nDst;
							else if( (nChess^nCaptured) >= 48 )
								*(ChessMove++) = (MvvValues[nCaptured]>>16) | (nSrc<<8) | nDst;
						}
						break;

					case 6:
						// 兵卒：纵向将军，横向逃离将军方向；横向将军，兵卒不能到达解将位置
						pMove = PawnMoves[Player][nSrc];
						while( *pMove )
						{
							nDst = *(pMove++);
							nCaptured = Board[nDst];
							
							if( !nCaptured && nDir0==16 && x != (nDst&0xF) )			// 横向逃离，禁止纵向移动
								*(ChessMove++) = (nSrc<<8) | nDst;
							else if( (nChess^nCaptured)>=48 )							// 纵向杀将军的炮
								*(ChessMove++) = (MvvValues[nCaptured]>>16) | (nSrc<<8) | nDst;
						}
						break;

					default:
						break;
				}
			}
			// 炮架子是对方的马，且形成马后炮之势，不能用移将法解将，到此为止，程序返回
			else if( nPaojiaziID0==OpKing+5 || nPaojiaziID0==OpKing+6 )
			{
				if( nKingSq-nPaojiazi0==2  || nKingSq-nPaojiazi0==-2 || 
					nKingSq-nPaojiazi0==32 || nKingSq-nPaojiazi0==-32 )
				return int(ChessMove-pGenMove);
			}

			break;


		// 炮车将军：
		case 0x05:  // 炮1车1
		case 0x06:  // 炮1车2
		case 0x09:  // 炮2车1
		case 0x0A:  // 炮2车2
			nCheckSq1 = Piece[ OpKing + (checkers&0x01 ? 1:2) ];
			nDir1 = (nKingSq&0xF)==(nCheckSq1&0xF) ? 16:1;
			nMin1 = nKingSq>nCheckSq1 ? nCheckSq1 : nKingSq+nDir1;
			nMax1 = nKingSq<nCheckSq1 ? nCheckSq1 : nKingSq-nDir1;

			// 车炮分别从两个方向将军，并且炮架子是对方棋子，无解
			if( nDir0!=nDir1 && (nPaojiaziID0-16)>>4==1-Player )
				return 0;
			// 炮架子是对方的马，且形成马后炮之势，无解
			else if( nPaojiaziID0==OpKing+5 || nPaojiaziID0==OpKing+6 )
			{
				if( nKingSq-nPaojiazi0==2  || nKingSq-nPaojiazi0==-2 || 
					nKingSq-nPaojiazi0==32 || nKingSq-nPaojiazi0==-32 )
				return 0;
			}
			// 若炮架子是对方的车，产生界于车将之间的非吃子移动移动，不包含车与将
			else if( nPaojiaziID0==OpKing+1 || nPaojiaziID0==OpKing+2 )
			{
				nMin0 = nKingSq>nPaojiazi0 ? nPaojiazi0+nDir0 : nKingSq+nDir0;
				nMax0 = nKingSq<nPaojiazi0 ? nPaojiazi0-nDir0 : nKingSq-nDir0;

				x = nDir0==1 ? MyKing+10 : MyKing+15;
				for(nDst=nMin0; nDst<=nMax0; nDst+=nDir0)
				{	
					// 车炮马象士兵，杀车或者挡车
					for(nChess=MyKing+1; nChess<=x; nChess++)
					{
						if( (nSrc=Piece[nChess]) != 0 )
							ChessMove += AddLegalMove(nChess, nSrc, nDst, ChessMove);
					}
				}
			}
			// 炮架子是己方的棋子(炮马象士)，车兵无法解将
			else if( nPaojiaziID0>=MyKing+3 && nPaojiaziID0<=MyKing+10 )
			{
				// 产生炮架子杀车或者阻挡车的移动
				nChess = Board[nPaojiazi0];
				for(nDst=nMin1; nDst<=nMax1; nDst+=nDir1)
					ChessMove += AddLegalMove(nChess, nPaojiazi0, nDst, ChessMove);
			}

			break;


		// 炮炮将军，炮炮车将军
		// "4ka2C/9/4b4/9/9/4C4/9/9/9/4K4 b - - 0 1"
		// "4kR2C/9/4b4/9/9/4C4/9/9/9/4K4 b - - 0 1"
		case 0x0C:	// 炮1炮2		
		case 0x0D:  // 炮1炮2车1
		case 0x0E:  // 炮1炮2车2
			// 若炮架子是对方的车，产生界于车将之间的移动
			if( nPaojiaziID0==OpKing+1 || nPaojiaziID0==OpKing+2 )
			{
				nMin0 = nKingSq>nPaojiazi0 ? nPaojiazi0+nDir0 : nKingSq+nDir0;
				nMax0 = nKingSq<nPaojiazi0 ? nPaojiazi0-nDir0 : nKingSq-nDir0;
			}
			

			nCheckSq1 = Piece[ OpKing + 4 ];
			nDir1 = (nKingSq&0xF)==(nCheckSq1&0xF) ? 16:1;
			nMin1 = nKingSq>nCheckSq1 ? nCheckSq1 : nKingSq+nDir1;
			nMax1 = nKingSq<nCheckSq1 ? nCheckSq1 : nKingSq-nDir1;

			// 寻找炮架子的位置
			for(nDst=nMin1+nDir1; nDst<nMax1; nDst+=nDir1)
			{
				if( Board[nDst] )
				{
					nPaojiazi1 = nDst;
					nPaojiaziID1 = Board[nDst];
					break;
				}
			}

			// 若炮架子是对方的车，产生界于车将之间的移动
			if( nPaojiaziID1==OpKing+1 || nPaojiaziID1==OpKing+2 )
			{
				nMin1 = nKingSq>nPaojiazi1 ? nPaojiazi1+nDir0 : nKingSq+nDir0;
				nMax1 = nKingSq<nPaojiazi1 ? nPaojiazi1-nDir0 : nKingSq-nDir0;
			}
			
			// 炮架子是己方的棋子：马、象、士
			if( nPaojiaziID0>=MyKing+5 && nPaojiaziID0<=MyKing+10 )
			{
				for(nDst=nMin1; nDst<=nMax1; nDst+=nDir1)
				{
					if( nDst==nPaojiazi1 )
						continue;
					ChessMove += AddLegalMove(nPaojiaziID0, nPaojiazi0, nDst, ChessMove);
				}
			}
			// 炮架子是己方的棋子：马、象、士
			if( nPaojiaziID1>=MyKing+5 && nPaojiaziID1<=MyKing+10 )	
			{
				for(nDst=nMin0; nDst<=nMax0; nDst+=nDir0)
				{
					if( nDst==nPaojiazi0 )
						continue;
					ChessMove += AddLegalMove(nPaojiaziID1, nPaojiazi1, nDst, ChessMove);
				}
			}

			// 炮架子是对方的马，且形成马后炮之势，移将法不能解将，到此返回
			if( nPaojiaziID0==OpKing+5 || nPaojiaziID0==OpKing+6 )
			{
				if( nKingSq-nPaojiazi0==2  || nKingSq-nPaojiazi0==-2 || 
					nKingSq-nPaojiazi0==32 || nKingSq-nPaojiazi0==-32 )
				return int(ChessMove-pGenMove);
			}

			// 炮架子是对方的马，且形成马后炮之势，移将法不能解将，到此返回
			if( nPaojiaziID1==OpKing+5 || nPaojiaziID1==OpKing+6 )
			{
				if( nKingSq-nPaojiazi1==2  || nKingSq-nPaojiazi1==-2 || 
					nKingSq-nPaojiazi1==32 || nKingSq-nPaojiazi1==-32 )
				return int(ChessMove-pGenMove);
			}

			break;

		// 炮马将军：
		case 0x14:	// 炮1马1
		case 0x18:  // 炮2马1
		case 0x24:	// 炮1马2
		case 0x28:  // 炮2马2
			// 炮架子是己方的棋子：车、炮、马、象、士，不包含将和兵
			if( nPaojiaziID0>=MyKing+1 && nPaojiaziID0<=MyKing+10 )
			{
				nCheckSq1 = Piece[ OpKing + (checkers&0x10 ? 5:6) ];
				ChessMove += AddLegalMove(nPaojiaziID0, nPaojiazi0, nCheckSq1, ChessMove);
				ChessMove += AddLegalMove(nPaojiaziID0, nPaojiazi0, nCheckSq1+nHorseLegTab[nKingSq-nCheckSq1+256], ChessMove);
			}
			break;


		// 炮马马将军：二马的马腿位置相同时，产生炮架子(车炮马士)绊马腿的移动
		// "3k1a2C/1r3N3/4N4/5n3/9/9/9/9/9/3K5 b - - 0 1"
		case 0x34:  // 炮1马1马2
		case 0x38:  // 炮2马1马2
			// 炮架子是己方的棋子：车、炮、马、象、士，不包含将和兵
			if( nPaojiaziID0>=MyKing+1 && nPaojiaziID0<=MyKing+10 )
			{
				nCheckSq0 = Piece[ OpKing + 5 ];
				nCheckSq1 = Piece[ OpKing + 6 ];
				nDst = nCheckSq0+nHorseLegTab[nKingSq-nCheckSq0+256];
				if( nDst==nCheckSq1+nHorseLegTab[nKingSq-nCheckSq1+256] )
					ChessMove += AddLegalMove(nPaojiaziID0, nPaojiazi0, nDst, ChessMove);
			}
			break;


		// 炮兵将军：炮架子吃兵；若炮架子是兵，只能移将解将
		case 0x44:  // 炮1纵兵
		case 0x48:  // 炮2纵兵
		case 0x84:  // 炮1横兵
		case 0x88:  // 炮2横兵
			// 炮架子是己方的棋子
			if( (nPaojiaziID0-16)>>4==Player )					
			{
				// 炮架子杀纵向之兵
				if( checkers<=0x48 )
					ChessMove += AddLegalMove(nPaojiaziID0, nPaojiazi0, nKingSq+(Player?-16:16), ChessMove);
				// 炮架子杀横向之兵
				else
				{
					nCheckSq0 = nKingSq-1;
					nCheckSq1 = nKingSq+1;
					// 左右都是兵，不能用这种方法解将
					if( Board[nCheckSq0]!=Board[nCheckSq1] )
					{
						// 炮架子吃左兵
						if( nPieceID[Board[nCheckSq0]]==6 )
							ChessMove += AddLegalMove(nPaojiaziID0, nPaojiazi0, nCheckSq0, ChessMove);
						// 炮架子吃右兵
						if( nPieceID[Board[nCheckSq1]]==6 )
							ChessMove += AddLegalMove(nPaojiaziID0, nPaojiazi0, nCheckSq1, ChessMove);
					}
				}
			}
			// 炮架子是对方的马，且形成马后炮之势，不能解将
			else if( nPaojiaziID0==OpKing+5 || nPaojiaziID0==OpKing+6 )
			{
				if( nKingSq-nPaojiazi0==2  || nKingSq-nPaojiazi0==-2 || 
					nKingSq-nPaojiazi0==32 || nKingSq-nPaojiazi0==-32 )
				return 0;
			}
			break;


		// 单马将军：吃马、绊马腿
		case 0x10:
		case 0x20:
			nCheckSq0 = Piece[ OpKing + (checkers&0x10 ? 5:6) ];
			for(nChess=MyKing+1; nChess<=MyKing+10; nChess++)
			{
				if( (nSrc=Piece[nChess]) != 0 )
					ChessMove += AddLegalMove(nChess, nSrc, nCheckSq0, ChessMove);
			}

			nDst = nCheckSq0+nHorseLegTab[nKingSq-nCheckSq0+256];
			for(nChess=MyKing+1; nChess<=MyKing+10; nChess++)
			{
				if( (nSrc=Piece[nChess]) != 0 )
					ChessMove += AddLegalMove(nChess, nSrc, nDst, ChessMove);
			}

			break;

		// 双马将军：二马的马腿位置相同时，产生绊马腿的移动
		case 0x30:
			nCheckSq0 = Piece[ OpKing + 5 ];
			nCheckSq1 = Piece[ OpKing + 6 ];
			nDst = nCheckSq0+nHorseLegTab[nKingSq-nCheckSq0+256];
			if( nDst == nCheckSq1+nHorseLegTab[nKingSq-nCheckSq1+256] )
			{
				// 车炮马象士
				for(nChess=MyKing+1; nChess<=MyKing+10; nChess++)
				{
					if( (nSrc=Piece[nChess]) != 0 )
						ChessMove += AddLegalMove(nChess, nSrc, nDst, ChessMove);
				}
			}
			break;

		// 纵向单兵：吃兵
		case 0x40:
			nDst = nKingSq+(Player?-16:16);
			// 车炮马象士
			for(nChess=MyKing+1; nChess<=MyKing+10; nChess++)
			{
				if( (nSrc=Piece[nChess]) != 0 )
					ChessMove += AddLegalMove(nChess, nSrc, nDst, ChessMove);
			}
			break;

		// 横向兵卒：吃兵
		case 0x80:
			nCheckSq0 = nKingSq-1;
			nCheckSq1 = nKingSq+1;

			// 左右都是兵，不能用这种方法解将
			if( Board[nCheckSq0]!=Board[nCheckSq1] )
			{
				// 吃左兵
				if( nPieceID[Board[nCheckSq0]]==6 )
				{
					// 车炮马象士
					for(nChess=MyKing+1; nChess<=MyKing+10; nChess++)
					{
						if( (nSrc=Piece[nChess]) != 0 )
							ChessMove += AddLegalMove(nChess, nSrc, nCheckSq0, ChessMove);
					}
				}

				// 吃右兵
				if( nPieceID[Board[nCheckSq1]]==6 )
				{
					// 车炮马象士
					for(nChess=MyKing+1; nChess<=MyKing+10; nChess++)
					{
						if( (nSrc=Piece[nChess]) != 0 )
							ChessMove += AddLegalMove(nChess, nSrc, nCheckSq1, ChessMove);
					}
				}
			}
			break;

		default:
			break;
	}

	// 移将法进行解将
	pMove = KingMoves[nKingSq];
	while( *pMove )
	{
		nDst = *(pMove++);
		nCaptured = Board[nDst];			
		
		if( !nCaptured && (													// 产生非吃子移动
			(!nDir0 && !nDir1) ||											// 没有车炮将军
			(nDir0!=1 && nDir1!=1 && (nKingSq&0xF0)==(nDst&0xF0)) ||		// 将横向移动，车炮不可横向将军
			(nDir0!=16 && nDir1!=16 && (nKingSq&0xF)==(nDst&0xF)) ) )		// 将纵向移动，车炮不可纵向将军
			*(ChessMove++) = (nKingSq<<8) | nDst;
		else if( (MyKing^nCaptured) >= 48 )					// 产生吃子移动
			*(ChessMove++) = (MvvValues[nCaptured]<<16) | (nKingSq<<8) | nDst;
	}
	
	return int(ChessMove-pGenMove);
}
