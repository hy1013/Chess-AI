////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 头文件：MoveGen.h                                                                                      //
// *******************************************************************************************************//
// 中国象棋通用引擎----简单测试引擎示例，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： wying                                                                                        //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 基础类型CMoveGen, CSearch子类继承之。棋盘、棋子、位行、位列、着法等数据在这个类中被定义。           //
// 2. 通用移动产生器                                                                                      //
// 3. 吃子移动产生器                                                                                      //
// 4. 将军逃避移动产生器                                                                                  //
// 5. 杀手移动合法性检验                                                                                  //
// 6. 将军检测Checked(Player), Checking(1-Player)                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "FenBoard.h"

#pragma once


// 棋盘数组和棋子数组
extern int Board[256];								// 棋盘数组，表示棋子序号：0～15，无子; 16～31,黑子; 32～47, 红子；
extern int Piece[48];								// 棋子数组，表示棋盘位置：0, 不在棋盘上; 0x33～0xCC, 对应棋盘位置；

// 位行与位列棋盘数组
extern unsigned int xBitBoard[16];					// 16个位行，产生车炮的横向移动，前12位有效
extern unsigned int yBitBoard[16];					// 16个位列，产生车炮的纵向移动，前13位有效

// 位行与位列棋盘的模
extern const int xBitMask[256];
extern const int yBitMask[256];

// 车炮横向与纵向移动的16位棋盘，只用于杀手移动合法性检验、将军检测和将军逃避   							          
extern unsigned short xBitRookMove[12][512];		//  12288 Bytes, 车的位行棋盘
extern unsigned short yBitRookMove[13][1024];		//  26624 Bytes  车的位列棋盘
extern unsigned short xBitCannonMove[12][512];		//  12288 Bytes  炮的位行棋盘
extern unsigned short yBitCannonMove[13][1024];	    //  26624 Bytes  炮的位列棋盘
									      // Total: //  77824 Bytes =  76K
extern unsigned short HistoryRecord[65535];		// 历史启发，数组下标为: move = (nSrc<<8)|nDst;

extern const char nHorseLegTab[512];
extern const char nDirection[512];

class CMoveGen
{
public:
	CMoveGen(void);
	~CMoveGen(void);

	//unsigned short HistoryRecord[65535];		// 历史启发，数组下标为: move = (nSrc<<8)|nDst;

// 调试信息
public:
	unsigned int nCheckCounts;
	unsigned int nNonCheckCounts;
	unsigned int nCheckEvasions;

// 方法
public:	
	// 更新历史记录，清零或者衰减
	void UpdateHistoryRecord(unsigned int nMode=0);

	// 通用移动产生器
	int MoveGenerator(const int player, CChessMove* pGenMove);

	// 吃子移动产生器
	int CapMoveGen(const int player, CChessMove* pGenMove);

	// 将军逃避移动产生器
	int CheckEvasionGen(const int Player, int checkers, CChessMove* pGenMove);
	
	// 杀手移动合法性检验
	int IsLegalKillerMove(int Player, const CChessMove KillerMove);

	// 将军检测，立即返回是否，用于我方是否被将军
	int Checked(int player);

	// 将军检测，返回将军类型，用于对方是否被将军
	int Checking(int Player);

	// 保护判断
	int Protected(int Player, int from, int nDst);

private:
	// 检验棋子piece是否能够从nSrc移动到nDst，若成功加入到走法队列ChessMove中
	int AddLegalMove(const int piece, const int nSrc, const int nDst, CChessMove *ChessMove);
};
