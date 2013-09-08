////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 头文件：HashTable.cpp                                                                                  //
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

#include <stdlib.h>			// 随机数发生器rand()函数的头文件
#include <stdio.h>			// 文件操作
#include <time.h>			// 初始化随机数
#include "HashTable.h"
#include "FenBoard.h"



CHashTable::CHashTable(void)
{
	//srand( (unsigned)time( 0 ) );		// 不使用时，每次都产生相同的局面值，以方便调试
}

CHashTable::~CHashTable(void)
{
}

void CHashTable::ZobristGen(void)
{
	int i, j;

	//ZobristKeyPlayer = Rand32U();

	//((unsigned long *) &ZobristLockPlayer)[0] = Rand32U();
	//((unsigned long *) &ZobristLockPlayer)[1] = Rand32U();

	for (i = 0; i < 14; i ++) 
	{
		for (j = 0; j < 256; j ++) 
		{
			ZobristKeyTable[i][j] = Rand32U();

			((unsigned long *) &ZobristLockTable[i][j])[0] = Rand32U();
			((unsigned long *) &ZobristLockTable[i][j])[1] = Rand32U();
		}
	}
}

//为Hash表分配内存：以后加入内存检测，优化大小，不成功则返回0
unsigned long CHashTable::NewHashTable(unsigned long nHashPower, unsigned long nBookPower)
{
	ZobristGen();

	nHashSize = 1<<(nHashPower-1); 
	nHashMask = nHashSize-1;						// 用 & 代替 % 运算

	pHashList[0]  = new CHashRecord[nHashSize];		// 黑方的Hash表
	pHashList[1]  = new CHashRecord[nHashSize];		// 红方的Hash表
	
	nMaxBookPos = 1<<nBookPower;
	pBookList = new CBookRecord[nMaxBookPos];

	return nHashSize;
}

// 释放内存
void CHashTable::DeleteHashTable()
{
	// 释放Hash表的内存
	delete [] pHashList[0];
	delete [] pHashList[1];

	// 释放开局库的内存
	delete [] pBookList;
}

// 清空Hash表，并且返回Hash表的覆盖率
// style==0，完全清除Hash记录，引擎启动或者新建棋局时，使用这个参数。
// style!=0，不完全清除，引擎使用(Hash.depth - 2)的策略，基于下面的原理：
// 1.按回合制，2ply个半回合后，轮到本方走棋。当前搜索树MaxDepth-2处，存在一颗子树，就是将要进行完全搜索的树枝，
// 其余的树枝都是无用的。相对于新的搜索而言，那颗子树少搜索了两层。把当前的Hash节点深度值减2，正好对应下一回合
// MaxDepth-2的子树。深度值减2以后，清Hash表之前depth=1和depth-2的的节点将不会命中，但依然能够为新的搜索树提供
// HashMove，也将节省一定的时间。根据深度优先覆盖策略，进行下一回合新的搜索，当迭代深度值depth<MaxDepth-2以前，
// 搜索树的主分支会直接命中，也不会覆盖任何Hash节点；之后，由于深度值的增加，将会逐渐覆盖相同局面的Hash节点。
// 2.若界面程序打开后台思考功能，新的搜索，即使后台思考没有命中，Hash表中积累的丰富信息同样能够利用。在规定的
// 时间内，引擎可以在此基础上搜索更深。
// 3.对于杀棋，Hash探察时不采取深度优先策略，清Hash表时，采取完全清除的方法，避免返回值不准确的问题。若使杀棋
// 的局面也能够利用，value<-MATEVALUE时，value+=2；value>MATEVALUE时，value-=2。若引擎独自红黑对战，则应该±1。
// 为了适应不同的搜索方式，增加安全性，采用保险的措施---完全清除。
// 4.清空Hash表，是个及其耗费时间的工作。所以选择引擎输出BestMove后进行，即“后台服务”。
// 5.测试表明，这种清除方案能够节省10～15%的搜索时间。
float CHashTable::ClearHashTable(int style)
{
	CHashRecord *pHash0 = pHashList[0];
	CHashRecord *pHash1 = pHashList[1];

	CHashRecord *pHashLimit = pHashList[0] + nHashSize;

	nHashCovers = 0;
	if( style )
	{
		while( pHash0 < pHashLimit )
		{
			// 计算Hash表的覆盖率
			//if(pHash0->flag)
			//	nHashCovers++;
			//if(pHash1->flag)
			//	nHashCovers++;

			// 遇到杀棋，HashFlag置0，表示完全清空
			if( (pHash0->value) < -MATEVALUE || (pHash0->value) > MATEVALUE )
				pHash0->flag = 0;
			if( (pHash1->value) < -MATEVALUE || (pHash1->value) > MATEVALUE )
				pHash1->flag = 0;

			// 深度值减 2，下一回合还可以利用
			(pHash0 ++)->depth -= 2;
			(pHash1 ++)->depth -= 2;
		}
	}
	else
	{
		while( pHash0 < pHashLimit )
		{
			(pHash0 ++)->flag = 0;
			(pHash1 ++)->flag = 0;
		}
	}

	return nHashCovers/(nHashSize*2.0f);
}

unsigned long CHashTable::Rand32U()
{
	//return rand() ^ ((long)rand() << 15) ^ ((long)rand() << 30);
	//许多资料使用上面的随机数，0<=rand()<=32767, 只能产生0～max(unsigned long)/2之间的随机数，显然很不均匀，会增加一倍的冲突几率

	return ((unsigned long)(rand()+rand())<<16) ^ (unsigned long)(rand()+rand());	//改进后应该是均匀分布的
}


//根据棋子的位置信息，初始化ZobristKey和ZobristLock
//当打开一个新的棋局或者载入棋局时，应当调用此函数
void CHashTable::InitZobristPiecesOnBoard(int *Piece)
{
	int m, n, chess;
	
	ZobristKey  = 0;
	ZobristLock = 0;

	for(n=16; n<48; n++)
	{
		m = Piece[n];
		if( m )
		{
			chess = nPieceType[n];

			ZobristKey  ^= ZobristKeyTable[chess][m];
			ZobristLock ^= ZobristLockTable[chess][m];
		}
	}
}

// 探察Hash表，若不成功返回"INT_MIN"；若发生冲突，则返回HashMove，标志为"INT_MAX"
// 散列法：
// 1. 相除散列法－－h(x) = ZobristKey % MaxHashZize =ZobristKey & (MaxHashSize-1)  M = MaxHashSize = 1<<k = 1024*1204, k=20 
//    用2的幂表示，可以把除法变成位(与)运算，速度很快。但是这正是散列表的大忌，2^k-1=11111111111111111111(B),仅仅用到了ZobristKey低20位信息，高12位信息浪费
//    由于采用了ZobristKey已经时均匀分布的随机数，相信相除散列法会工作得很好。唯一的缺点是存在奇偶现象，即h(x) 与 ZobristKey的奇偶相同。
// 2. 平方取中散列法－－h(x)=[M/W((x*x)%W)] = (x*x)>>(w-k) = (x*x)>>(32-20) = (x*x)>>12  前行零x<sqrt(W/M)和尾随零x=n*sqrt(W)的关键字会冲突
//    把x*x右移动12位，剩下左面20位，能够产生0～M-1之间的数。
// 3. Fibonacci(斐波纳契数列)相乘散列法, h(x) = (x*2654435769)>>(32-k)  2654435769是个质数，且2654435769/2^32 = 0.618 黄金分割点，即使连续的键值都能均匀分布
//    2^16  40503
//    2^32  2654435769					倒数： 340573321   a%W
//    2^64  11400714819323198485
// CHashRecord *pHashIndex = pHashList[player] + ((ZobristKey * 2654435769)>>(32 - nHashPower));		// Fibonacci(斐波纳契数列)相乘散列法
// 经试验，相除散列法运行时间最快。Fibonacci主要涉及到复杂的乘法和位移运算，故而速度反而不如简单的方法。
int CHashTable::ProbeHash(CChessMove &hashmove, int alpha, int beta, int nDepth, int ply, int player)
{
	CHashRecord HashIndex = pHashList[player][ZobristKey & nHashMask];								//找到当前棋盘Zobrist对应的Hash表的地址
	
	if( HashIndex.zobristlock == ZobristLock )				//已经存在Hash值, 是同一棋局
	{	
		if( HashIndex.flag & HashExist )
		{
			// 修正将军局面的Hash值，即使这样，电脑仍然不能找到最短路线，Hash表命中后的步法失去准确性。
			// 但是距离将军的分数是正确的，更为可观的是，残局时搜索速度快30～40%.
			// 将军的着法与深度无关，无论在哪层将军，只要盘面相同，便可以调用Hash表中的值。
			bool bMated = false;
			if( HashIndex.value > MATEVALUE )				// 获胜局面
			{
				bMated = true;
				HashIndex.value -= ply;						// 减去当前的回合数，即score = WINSCORE-Hash(ply)-ply, 需要更多的步数才能获胜，如此能够得到最短的将军路线
			}
			else if( HashIndex.value < -MATEVALUE )			// 失败局面
			{
				bMated = true;
				HashIndex.value += ply;						// 加上当前的回合数，即score = Hash(ply)+ply-WINSCORE, 如此电脑会争取熬更多的回合，顽强抵抗，等待人类棋手走出露着而逃避将军
			}

			if( HashIndex.depth >= nDepth || bMated )		// 靠近根节点的值更精确，深度优先；将军局面，不管深度，只要能正确返回距离胜败的回合数即可
			{
				//配合深度迭代时会出现问题，必须每次都清除Hash表。
				//因为采用深度优先覆盖，把靠近根节点的值当成最精确的。
				//但是以前的浅层搜索的估值是不精确的，深度优先时应该抛弃这些内容。
				if(HashIndex.flag & HashBeta)			
				{
					if (HashIndex.value >= beta)
					{
						nHashBeta++;					// 95-98%
						return HashIndex.value;
					}
				}
				else if(HashIndex.flag & HashAlpha)		// 也能减少一部分搜索，从而节省时间
				{
					if (HashIndex.value <= alpha)	
					{
						nHashAlpha++;					// 2-5%
						return HashIndex.value;
					}
				}
				else if(HashIndex.flag & HashPv)		// 
				{
					nHashExact++;						// 0.1-1.0%
					return HashIndex.value;
				}

				// 剩下很少量的分支，是一些刚刚展开的分支(跟踪发现，其数目与HashAlpha相近)，beta=+32767, 上面的的探察会失败
			}
		}

		// 对于NullMove移动和叶节点，没有HashMove。
		if( !HashIndex.move )
			return(INT_MIN);

		// Hash表未命中，但局面相同，产生HashMove。
		// 也可能是上一回合遗留下来的移动，清除Hash表时，只是把flag=0，HashMove仍然保留在记录中。
		// 试验发现：以后多次搜索同一局面，时间会越来越少，最终趋于平衡。
		if( HashIndex.depth > 0 )
		{
			nCollision ++;
			hashmove = HashIndex.move;
			return INT_MAX;		// 表示相同局面的陈旧Hash值
		}
	}

	return(INT_MIN);		// 不是相同的棋局
}

void CHashTable::RecordHash(const CChessMove hashmove, int score, int nFlag, int nDepth, int ply, int player)
{
	//找到当前棋盘Zobrist对应的Hash表的地址
	CHashRecord *pHashIndex = pHashList[player] + (ZobristKey & nHashMask);

	// 过滤，深度优先原则
	if((pHashIndex->flag & HashExist) && pHashIndex->depth > nDepth)
		return;

	// 过滤循环局面。循环局面不能写入Hash表！
	//     假如某条路线存在循环，那么搜索树返回到循环的初始局面时，路线的每个局面都将被存入Hash表，不是赢棋就是输棋。
	// 再次搜索时，又遇到这条路线上的棋局，但没有走完整条循环路线，理论上就不构成循环。但程序会从Hash表中发现，走这条
	// 路线会赢棋或者输棋。如果是输棋，程序则会被以前的局面吓倒，绝不敢踏上这条路。实际上，走两次是可以的，但不能走三次，
	// 因为三次就构成了循环。
	//    残局时，这种情况屡见不鲜。有些局面，程序走这种路线会赢得胜利；弱势时可以顽强反抗。
	//    这又涉及到循环的返回值问题。实际上，对于不是和棋的局面，循环的着法从来不是最顽强的应对方式。即使弱势情况，
	// 不使用循环应对才可以维持得更久。基于这个原理，循环局面可以返回小于-WINSCORE或者大于+WINSCORE的值。返回到搜索树
	// 的上一层，RecordHash()函数就可以利用这个值来判断出现了循环的局面。于是不记录在Hash表中，只是简单的返回。对后续的
	// 搜索不会造成影响。
	if(score<-WINSCORE || score>+WINSCORE)
		return;

	// 修正将军局面的Hash值，可以获得最短的将军路线。
	if( score > MATEVALUE )
		score += ply;
	else if( score < -MATEVALUE )
		score -= ply;

	// 记录Hash值
	pHashIndex->zobristlock = ZobristLock;
	pHashIndex->flag    	= (unsigned char)nFlag;
	pHashIndex->depth   	= (char)nDepth;
	pHashIndex->value       = (short)score;
	pHashIndex->move        = (unsigned short)hashmove;		//仅保存后16位的信息
}

// 开局库格式：
// 移动 权重 局面，例b2e2 5895 rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1
int CHashTable::LoadBook(const char *BookFile) 
{
	FILE *FilePtr;
	char *LinePtr;
	char LineStr[256];
	CChessMove  BookMove, temp;
	CHashRecord TempHash;
	CFenBoard fen;

	int Board[256], Piece[48];
	int Player;
	unsigned int nNonCapNum;
	unsigned int nCurrentStep;

	// 下面的两个临时变量用来保存旧的棋局，函数结束时用来恢复；
	// 因为整理开局库时，会改变Hash表中当前的局面，否则引擎不能恢复原来的局面。
	unsigned long    Old_ZobristKey = ZobristKey;
	unsigned __int64 Old_ZobristLock = ZobristLock;

	if(!BookFile)
		return 0;

	// 以只读文本方式打开开局库
	if((FilePtr = fopen(BookFile, "rt"))==0)
		return 0;										// 不成功，返回0

	nBookPosNum = 0;
	LineStr[254] = LineStr[255] = '\0';
	
	// 从开局库中读取254个字符
	while(fgets(LineStr, 254, FilePtr) != 0)
	{
		// 移动
		LinePtr = LineStr;
		BookMove = Move(*(long *) LinePtr);				// 将移动从字符型转化为数字型，BookMove低16位有效

		if( BookMove )
		{
			// 权重
			LinePtr += 5;								// 跳过着法的4个字符和1个空格
			temp = 0;
			while(*LinePtr >= '0' && *LinePtr <= '9')
			{
				temp *= 10;
				temp += *LinePtr - '0';
				LinePtr ++;
			}
			
			BookMove |= temp<<16;						// 将此移动的权值(得分)保存在Book的高16位，低16位是移动的着法

			// 局面
			LinePtr ++;														// 跳过空格
			fen.FenToBoard(Board, Piece, Player, nNonCapNum, nCurrentStep, LinePtr);	// 把LinePtr字符串转化为棋盘数据
			InitZobristPiecesOnBoard( Piece );								// 根据棋盘上的棋子计算ZobristKey和ZobristLock

			if( Board[(BookMove & 0xFF00)>>8] )								// 此位置有棋子存在
			{
				TempHash = pHashList[Player][ZobristKey & nHashMask];
				if(TempHash.flag)											// Hash表中有内容
				{
					if(TempHash.zobristlock == ZobristLock)					// 而且是相同的局面
					{
						if(TempHash.flag & BookUnique)						// 开局库中是唯一着法
						{
							if(nBookPosNum < nMaxBookPos)					// 没有超出开局库的范围
							{
								TempHash.zobristlock = ZobristLock;
								TempHash.flag = BookMulti;
								TempHash.value = (short)nBookPosNum;

								pBookList[nBookPosNum].MoveNum = 2;
								pBookList[nBookPosNum].MoveList[0] = (TempHash.value<<16) | TempHash.move;
								pBookList[nBookPosNum].MoveList[1] = BookMove;
								
								nBookPosNum ++;
								pHashList[Player][ZobristKey & nHashMask] = TempHash;
							}
						} 
						else															// 开局库中有两个以上的变招 
						{
							if(pBookList[TempHash.value].MoveNum < MaxBookMove)
							{
								pBookList[TempHash.value].MoveList[pBookList[TempHash.value].MoveNum] = BookMove;
								pBookList[TempHash.value].MoveNum ++;
							}
						}
					}
				} 
				else					// Hash表中没有当前的局面，写入BestMove
				{
					TempHash.zobristlock = ZobristLock;
					TempHash.flag = BookUnique;
					TempHash.move = unsigned short(BookMove & 0xFFFF);
					TempHash.value = unsigned short(BookMove >> 16);
					pHashList[Player][ZobristKey & nHashMask] = TempHash;
				}
			}
		}
	}
	fclose(FilePtr);

	// 恢复棋局，引擎可以接着原来的棋局继续对弈。
	ZobristKey = Old_ZobristKey;
	ZobristLock = Old_ZobristLock;

	return 1;
}



int CHashTable::ProbeOpeningBook(CChessMove &BookMove, int Player)
{
	CHashRecord TempHash = pHashList[Player][ZobristKey & nHashMask];
	
	if((TempHash.flag & BookExist) && TempHash.zobristlock == ZobristLock)
	{
		if(TempHash.flag & BookUnique)			// 开局库中存在唯一的着法，命中。
		{
			BookMove = (TempHash.value<<16) | TempHash.move;
			return INT_MAX;
		} 
		else
		{			
			CBookRecord *pBookIndex = pBookList + TempHash.value;

			int m, ThisValue = 0;
			for(m=0; m<pBookIndex->MoveNum; m++)
				ThisValue += (pBookIndex->MoveList[m] & 0xFFFF0000) >> 16;

			if(ThisValue) 
			{
				ThisValue = Rand32U() % ThisValue;
				for(m=0; m<pBookIndex->MoveNum; m++)
				{
					ThisValue -= (pBookIndex->MoveList[m] & 0xFFFF0000) >> 16;

					if( ThisValue < 0 ) 
						break;
				}

				BookMove = pBookIndex->MoveList[m];
				return INT_MAX;
			}
		}
	}

	return 0;
}