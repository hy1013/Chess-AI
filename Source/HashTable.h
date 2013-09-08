////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 头文件：HashTable.h                                                                                    //
// *******************************************************************************************************//
// 中国象棋通用引擎----简单测试引擎示例，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： wying                                                                                        //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 为引擎分配Hash表，1～1024MB                                                                         //
// 2. Hash探察 与 Hash存储                                                                                //
// 3. 初始化开局库                                                                                        //
// 4. 查找开局库中的着法                                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "FenBoard.h"

#pragma once


const int WINSCORE = 10000;				// 胜利的分数WINSCORE，失败为-WINSCORE
const int MATEVALUE = 9000;				// score<-MATEVALUE || score>+MATEVALUE, 说明是个将军的局面


// 常量
const int BookUnique = 1;
const int BookMulti = 2;
const int BookExist = BookUnique | BookMulti;
const int HashAlpha = 4;
const int HashBeta = 8;
const int HashPv = 16;
const int HashExist = HashAlpha | HashBeta | HashPv;
const int MaxBookMove = 15;

#define INT_MIN  (-2147483648)  //const int INT_MIN = -2147483648;//-2147483647-1;   //wyingdebug
#define INT_MAX  (2147483647)  //const int INT_MAX = 2147483647;
#define SHRT_MAX  (32767) //const int SHRT_MAX = 32767;

// 开局库着法结构
struct CBookRecord
{
	int MoveNum;
	CChessMove MoveList[MaxBookMove];
};

// 哈西表记录结构
struct CHashRecord					// 14Byte --> 16Byte
{
	unsigned __int64   zobristlock;	// 8 byte  64位标识
	unsigned char      flag;        // 1 byte  flag==0, Hash值被清除
	char			   depth;		// 1 byte  搜索深度
	short			   value;       // 2 byte  估值
	unsigned short     move;		// 2 byte
};


// Hash表类
class CHashTable
{
public:
	CHashTable(void);
	virtual ~CHashTable(void);

// 
public:
	//unsigned long    ZobristKeyPlayer;
	//unsigned __int64 ZobristLockPlayer;

	unsigned long    ZobristKey;
	unsigned __int64 ZobristLock;

	unsigned long    ZobristKeyTable[14][256];
	unsigned __int64 ZobristLockTable[14][256];

	unsigned long  nHashSize;			// Hash表的实际大小
	unsigned long  nHashMask;			// nHashMask = nHashSize-1;
	CHashRecord    *pHashList[2];		// Hash表，黑红双方各用一个，以免发生冲突，这样解决最彻底
										// 参考王晓春著《PC游戏编程----人机博弈.pdf》

	unsigned int   nMaxBookPos;
	unsigned int   nBookPosNum;
	CBookRecord    *pBookList;


// 调试信息
public:
	unsigned long nCollision;	//Hash冲突计数器
	unsigned long nHashAlpha, nHashExact, nHashBeta;
	unsigned long nHashCovers;


public:
	// 为Hash表分配和开局库分配内存
	unsigned long NewHashTable(unsigned long nHashPower, unsigned long nBookPower);

	// 清除Hash表和开局库
	void DeleteHashTable();

	// Hash表清零，并且返回Hash表的覆盖率
	float ClearHashTable(int style=0);

	// 初始化与Hash表相关的棋子数据
	void InitZobristPiecesOnBoard(int *Piece);

	// Hash探察
	int ProbeHash(CChessMove &hashmove, int alpha, int beta, int nDepth, int ply, int player);

	// Hash存储
	void RecordHash(const CChessMove hashmove, int score, int nFlag, int nDepth, int ply, int player);
	
	// 初始化开局库
	int LoadBook(const char *BookFile);

	// 在开局库中寻找着法
	int ProbeOpeningBook(CChessMove &BookMove, int Player);

private:
	// 32位随机数发生器
	unsigned long Rand32U();

	// 初始化ZobristKeyTable[14][256]和ZobristLockTable[14][256]，赋予32位和64位随即数
	// 只需一次，在CHashTable()构造函数中自动进行，无需引擎调用
	// 除非重新启动程序，否则随即数将不会改变
	void ZobristGen(void);
};
