////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 头文件：FenBoard.h                                                                                     //
// *******************************************************************************************************//
// 中国象棋通用引擎----简单测试引擎示例，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： wying                                                                                        //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 将fen串转化为棋盘信息                                                                               //
// 2. 将棋盘信息转化为fen串                                                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#define  CChessMove  unsigned int


const int nPieceType[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 无子
	                       0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  6,  6,  6,		// 黑子：帅车炮马象士卒
					       7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 13, 13, 13  };	// 红子：将车炮马相仕兵

const int nPieceID[] = {  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,		// 无子
	                       0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  6,  6,  6,		// 黑子：帅车炮马象士卒
					       0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  6,  6,  6  };	// 红子：将车炮马相仕兵

class CFenBoard
{
public:
	CFenBoard(void);
	~CFenBoard(void);

public:	
	// 传递棋局的字符串。字符串必须足够长，否则当回合数太大时，moves超出，与棋盘相互串换时会发生死循环。
	char FenStr[2048];

public:
	// 将fen串转化为棋局信息，返回成功或失败的标志
	int FenToBoard(int *Board, int *Piece, int &Player, unsigned int &nNonCapNum, unsigned int &nCurrentStep, const char *FenStr);

	// 将当前棋局转化为fen串，返回串的指针
	char *BoardToFen(const int *Board, int Player, const unsigned int nNonCapNum=0, const unsigned int nCurrentStep=1, CChessMove *StepRecords=0);

private:
	// 将fen字符转化为棋子序号
	int FenToPiece(char fen);	
};


inline unsigned int Coord(const CChessMove move)
{
    unsigned char RetVal[4];
	unsigned int  src = (move & 0xFF00) >> 8;
	unsigned int  dst = move & 0xFF;

	RetVal[0] = unsigned char(src & 0xF) -  3 + 'a';
	RetVal[1] = 12 - unsigned char(src >> 4 ) + '0';
	RetVal[2] = unsigned char(dst & 0xF) -  3 + 'a';
	RetVal[3] = 12 - unsigned char(dst >> 4 ) + '0';

	return *(unsigned int *) RetVal;
}

inline CChessMove Move(const unsigned int MoveStr) 
{
	unsigned char *ArgPtr = (unsigned char *) &MoveStr;
	unsigned int src = ((12-ArgPtr[1]+'0')<<4) + ArgPtr[0]-'a'+3;	// y0x0
	unsigned int dst = ((12-ArgPtr[3]+'0')<<4) + ArgPtr[2]-'a'+3;	// y1x1
	return ( src << 8 ) | dst;										// y0x0y1x1
}