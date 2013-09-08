////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 头文件：Search.h                                                                                       //
// *******************************************************************************************************//
// 中国象棋通用引擎----简单测试引擎示例，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： IwfWcf                                                                                        //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 作为CMoveGen的子类，继承父类的数据，如棋盘、棋子、着法等。                                          //
// 2. 接收界面的数据，初始化为搜索需要的信息。                                                            //
// 3. 调用CHashTable类，执行和撤销着法。                                                                  //
// 4. 采用冒泡法对着法排序                                                                                //
// 5. 主控搜索函数 MainSearch()                                                                           //
// 6. 根节点搜索控制 RootSarch()                                                                          //
// 7. 博弈树搜索算法 AlphaBetaSearch()                                                                    //
// 8. 寂静搜索算法 QuiescenceSearch()                                                                     //
// 9. 基于1999年版《中国象棋竞赛规则》实现循环检测                                                        //
//10. 循环的返回值问题、与Hash表冲突问题。                            //
//11. 利用Hash表获取主分支                                                                                //
//12. 搜索时间的控制                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <utility>
#include <algorithm>
#include "MoveGen.h"
#include "HashTable.h"
#include "Evaluation.h"

using namespace std;

#define X first
#define Y second
#define MP make_pair

#pragma once

typedef pair<int, int> PII;

// 
const int MaxKiller = 4;
struct CKillerMove
{
	int MoveNum;
	CChessMove MoveList[MaxKiller];
};


// CSearch类
class CSearch : public CMoveGen
{
public:
	CSearch(void);
	~CSearch(void);

// 属性
	FILE *OutFile;
	CFenBoard fen;

	int Player;								// 当前移动方	
	int MaxDepth;							// 最大搜索深度
	unsigned long BitPieces;				// 32颗棋子的位图：0-15为黑棋，16-31为红棋  0 11 11 11 11 11 11111 0 11 11 11 11 11 11111
	
	CHashTable m_Hash;			// 哈希表
	CEvaluation m_Evalue;

	// 与ucci协议通讯的变量
	int bPruning;							// 是否允许使用NullMove
	int bKnowledge;							// 估值函数的使用
	int nSelectivity;						// 选择性系数，通常有0,1,2,3四个级别，缺省为0
	int Debug;								// bDegug = ~0, 输出详细的搜索信息
	int SelectMask;							// 选择性系数
	int nStyle;								// 保守(0)、普通(1)、冒进(2)
	int Ponder;								// 后台思考
	int bStopThinking;						// 停止思考
	int QhMode;								// 引擎是否使用浅红协议＝否
	int bBatch;								// 和后台思考的时间策略有关
	int StyleShift;
	long nMinTimer, nMaxTimer;				// 引擎思考时间
	int bUseOpeningBook;					// 是否让引擎使用开局库
	int nMultiPv;							// 主要变例的数目
	int nBanMoveNum;						// 禁手数目
	CChessMove BanMoveList[111];			// 禁手队列
	int NaturalBouts;						// 自然限着
	
	
	unsigned int StartTimer, FinishTimer;	// 搜索时间
	unsigned int nNonCapNum;				// 走法队列未吃子的数目，>=120(60回合)为和棋，>=5可能出现循环
	unsigned int nStartStep;
	unsigned int nCurrentStep;				// 当前移动的记录序号
	
	unsigned int nPvLineNum;
	CChessMove PvLine[64];					// 主分支路线
	CChessMove StepRecords[256];			// 走法记录
	unsigned int nZobristBoard[256];
	PII Record[111];
	PII Score[111];
	

	int nFirstLayerMoves;
	CChessMove FirstLayerMoves[111];		// fen C8/3P1P3/3kP1N2/5P3/4N1P2/7R1/1R7/4B3B/3KA4/2C6 r - - 0 1	// 将军局面    way = 111;
											// fen C8/3P1P3/4k1N2/3P1P3/4N1P2/7R1/1R7/4B3B/3KA4/2C6 r - - 0 1	// 非将军局面  way = 110;


// 杂项
public:
	unsigned int nTreeNodes;	
	unsigned int nLeafNodes;	
	unsigned int nQuiescNodes;

	unsigned int nTreeHashHit;
	unsigned int nLeafHashHit;

	unsigned int nNullMoveNodes;
	unsigned int nNullMoveCuts;

	unsigned int nHashMoves;
	unsigned int nHashCuts;	

	unsigned int nKillerNodes[MaxKiller];
	unsigned int nKillerCuts[MaxKiller];

	unsigned int nCapCuts;
	unsigned int nCapMoves;

	unsigned int nBetaNodes;
	unsigned int nPvNodes;
	unsigned int nAlphaNodes;

	
	unsigned int nZugzwang;

	//char FenBoard[2048];	// 传递棋局的字符串。字符串必须足够长，否则当回合数太大时，moves超出，与棋盘相互串换时会发生死循环。


// 操作
public:
	void InitBitBoard(const int Player, const int nCurrentStep);		// 初始化棋盘需要的所有数据，返回我方被将军的标志。

	int MovePiece(const CChessMove move);	 //&---可以大幅度提高搜索速度，与复制源代码速度相等，inline不起作用。
	void UndoMove(void);

	//void BubbleSortMax(CChessMove *ChessMove, int w, int way);	//冒泡排序：只需遍历一次，寻找最大值，返回最后一次交换记录的位置

	int MainSearch(int nDepth, long nProperTimer=0, long nLimitTimer=0);
	//int RootSearch(int depth, int alpha, int beta);
	int SimpleSearch(int depth, int alpha, int beta);
	PII PrincipalVariation(int depth, int alpha, int beta);
	PII NegaScout(int depth, int alpha, int beta);
	PII FAlphaBeta(int depth, int alpha, int beta);
	//int AlphaBetaSearch(int depth, int bDoCut, CKillerMove &KillerTab, int alpha, int beta);
	//int QuiescenceSearch(int depth, int alpha, int beta);	
	
	int RepetitionDetect(void);								// 循环检测
	int LoopValue(int Player, int ply, int nLoopStyle);		// 循环估值

	//int Evaluation(int player);

	int IsBanMove(CChessMove move);
	void GetPvLine(void);
	void PopupInfo(int depth, int score, int Debug=0);
	int Interrupt(void);


	char *GetStepName(const CChessMove ChessMove, int *Board) const;
	void SaveMoves(char *FileName);
};
