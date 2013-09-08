////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 头文件：PreMove.h                                                                                      //
// *******************************************************************************************************//
// 中国象棋通用引擎----简单测试引擎示例，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： wying                                                                                        //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 初始化棋子（将车炮马象士兵）的移动，产生伪合法的着法。                                              //
// 2. 初始化车炮位行与位列棋盘                                                                            //
//                                                                                                        //
// 注：CPreMove类不包含实际的着法数组，初始化完成后，即可释放此类，程序启动后只需调用一次                 //
////////////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

static const char nBoardIndex[256] = 
{
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

const int nCityIndex[256] = 
{
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};


// 移动预产生类
class CPreMove
{
public:
	CPreMove(void);
	virtual ~CPreMove(void);


// 着法预产生
public:	
	void InitKingMoves(unsigned char KingMoves[][8]);

	void InitRookMoves(unsigned char xRookMoves[][512][12], unsigned char yRookMoves[][1024][12], unsigned char xRookCapMoves[][512][4], unsigned char yRookCapMoves[][1024][4]);
	void InitCannonMoves(unsigned char xCannonMoves[][512][12], unsigned char yCannonMoves[][1024][12], unsigned char xCannonCapMoves[][512][4], unsigned char yCannonCapMoves[][1024][4]);

	void InitKnightMoves(unsigned char KnightMoves[][12]);
	void InitBishopMoves(unsigned char BishopMoves[][8]);

	void InitGuardMoves(unsigned char GuardMoves[][8]);
	void InitPawnMoves(unsigned char PawnMoves[][256][4]);



// 车炮横向与纵向移动的位棋盘
public:						
	void InitBitRookMove( unsigned short xBitRookMove[][512], unsigned short yBitRookMove[][1024] );
	void InitBitCannonMove( unsigned short xBitCannonMove[][512], unsigned short yBitCannonMove[][1024] );
	void InitBitSupperCannon( unsigned short xBitSupperCannon[][512], unsigned short yBitSupperCannon[][1024] );
};

