////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 源文件：PreMove.cpp                                                                                    //
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

#include "PreMove.h"


CPreMove::CPreMove()
{
}

CPreMove::~CPreMove()
{
}


// 将帅的移动，不包括双王照面
// 初始化为将帅移动的目标格，nDst = *pMove
// 所有的非俘获移动都合法，俘获移动不一定合法，遇到吃自己的棋子应加以判别
void CPreMove::InitKingMoves(unsigned char KingMoves[][8])
{
	int i, m, n;
	unsigned char *pMove;
	int nKingMoveDir[4] = { -16, -1, 1, 16 };

	for(m=0x36; m<0xC9; m++)
	{
		if( nCityIndex[m] )										//在九宫内
		{
			pMove = KingMoves[m];
			for(i=0; i<4; i++)
			{
				n = m + nKingMoveDir[i];
				if( nCityIndex[n] )
					*(pMove++)  = unsigned char( n );
			}
			*pMove = 0;		// 表示不能再移动
		}
	}
}


// 初始化车的着法预产生数组：
// 其值为行/列的相对位置x/y*16，计算目标格子的公式为：nDst(y|x) = (nSrc & 0xF0) | x; 或者 nDst(y|x) = y | (nSrc & 0xF);  // x = *pMove; y = *pMove;
// 这一点与将、士、象、马、兵的表示为棋子的绝对位置方式不同
// 所有的非俘获移动都合法，俘获移动不一定合法，遇到吃自己的棋子应加以判别
void CPreMove::InitRookMoves(unsigned char xRookMoves[][512][12], unsigned char yRookMoves[][1024][12], unsigned char xRookCapMoves[][512][4], unsigned char yRookCapMoves[][1024][4])
{
	int m, n, x, y;
	unsigned char *pMove, *pCapMove;

	// 初始化车的横向移动
	for(x=3; x<12; x++)
	{
		for(m=0; m<512; m++)
		{
			pMove    = xRookMoves[x][m];
			pCapMove = xRookCapMoves[x][m];
			for(n=x-1; n>=3; n--)					// 向左搜索
			{
				*(pMove++) = unsigned char( n );
				if( m & ( 1 << (n-3) ) )			// 遇到棋子，产生俘获
				{
					(*pCapMove++) = *(pMove-1);
					break;
				}
			}

			for(n=x+1; n<12; n++)					// 向右搜索
			{
				*(pMove++) = unsigned char( n );
				if( m & ( 1 << (n-3) ) )			// 遇到棋子，产生俘获
				{
					(*pCapMove++) = *(pMove-1);
					break;
				}
			}

			*pMove    = 0;
			*pCapMove = 0;
		}
	}

	// 初始化车的纵向移动
	for(y=3; y<13; y++)
	{
		for(m=0; m<1024; m++)
		{
			pMove    = yRookMoves[y][m];
			pCapMove = yRookCapMoves[y][m];
			for(n=y-1; n>=3; n--)
			{
				*(pMove++) = unsigned char( n<<4 );
				if( m & ( 1 << (n-3) ) )			// 遇到棋子，产生俘获
				{
					(*pCapMove++) = *(pMove-1);
					break;
				}
			}
			for(n=y+1; n<13; n++)
			{
				*(pMove++) = unsigned char( n<<4 );
				if( m & ( 1 << (n-3) ) )			// 遇到棋子，产生俘获
				{
					(*pCapMove++) = *(pMove-1);
					break;
				}
			}
			
			*pMove    = 0;
			*pCapMove = 0;
		}
	}
}


// 初始化炮的着法预产生数组：
// 其值为行/列的相对位置x/y*16，计算目标格子的公式为：nDst(y|x) = (nSrc & 0xF0) | x; 或者 nDst(y|x) = y | (nSrc & 0xF);  // x = *pMove; y = *pMove;
// 这一点与将、士、象、马、兵的表示为棋子的绝对位置方式不同
// 所有的非俘获移动都合法，俘获移动不一定合法，遇到吃自己的棋子应加以判别
void CPreMove::InitCannonMoves(unsigned char xCannonMoves[][512][12], unsigned char yCannonMoves[][1024][12], unsigned char xCannonCapMoves[][512][4], unsigned char yCannonCapMoves[][1024][4])
{
	int m, n, x, y;
	unsigned char *pMove, *pCapMove;

	//初始化炮的横向移动
	for(x=3; x<12; x++)
	{
		for(m=0; m<512; m++)
		{
			pMove    = xCannonMoves[x][m];
			pCapMove = xCannonCapMoves[x][m];
			for(n=x-1; n>=3; n--)
			{
				if( m & ( 1 << (n-3) ) )			// 遇到炮架子
				{
					n--;							// 跨越炮架子
					break;
				}
				*(pMove++) = unsigned char( n );
			}
			for(; n>=3; n--)
			{
				if( m & ( 1 << (n-3) ) )			// 遇到棋子才移动
				{
					*(pMove++) = unsigned char( n );
					*(pCapMove++) = *(pMove-1);
					break;
				}
			}

			for(n=x+1; n<12; n++)
			{
				if( m & ( 1 << (n-3) ) )			// 遇到炮架子
				{
					n++;							// 跨越炮架子
					break;
				}
				*(pMove++) = unsigned char( n );
			}
			for(; n<12; n++)
			{
				if( m & ( 1 << (n-3) ) )			// 遇到棋子才移动
				{
					*(pMove++) = unsigned char( n );
					*(pCapMove++) = *(pMove-1);
					break;
				}
			}

			*pMove    = 0;
			*pCapMove = 0;	
		}
	}

	//初始化炮的纵向移动
	for(y=3; y<13; y++)
	{
		for(m=0; m<1024; m++)
		{
			pMove    = yCannonMoves[y][m];
			pCapMove = yCannonCapMoves[y][m];
			for(n=y-1; n>=3; n--)
			{
				if( m & ( 1 << (n-3) ) )			//遇到炮架子
				{
					n--;							//跨越炮架子
					break;
				}
				*(pMove++) = unsigned char( n<<4 );
			}
			for(; n>=3; n--)
			{
				if( m & ( 1 << (n-3) ) )			//遇到棋子才移动
				{
					*(pMove++) = unsigned char( n<<4 );
					*(pCapMove++) = *(pMove-1);
					break;
				}
			}

			for(n=y+1; n<13; n++)
			{
				if( m & ( 1 << (n-3) ) )			//遇到炮架子
				{
					n++;							//跨越炮架子
					break;
				}
				*(pMove++) = unsigned char( n<<4 );
			}
			for(; n<13; n++)
			{
				if( m & ( 1 << (n-3) ) )			//遇到棋子才移动
				{
					*(pMove++) = unsigned char( n<<4 );
					*(pCapMove++) = *(pMove-1);
					break;
				}
			}

			*pMove    = 0;
			*pCapMove = 0;
		}
	}
}


// 初始化马的着法预产生数组：
// 其值初始化为马移动的绝对位置。
// 不合法移动：绊马腿的移动，吃己方棋子的移动，必须检测。
void CPreMove::InitKnightMoves(unsigned char KnightMoves[][12])
{
	int i, m, n;
	unsigned char *pMove;
	int nHorseMoveDir[] = { -33, -31, -18, -14, 14, 18, 31, 33 };
	
	for(m=0x33; m<0xCC; m++)
	{
		if( nBoardIndex[m] )
		{
			pMove = KnightMoves[m];
			for(i=0; i<8; i++)
			{	
				n = m + nHorseMoveDir[i];
				if( nBoardIndex[n] )						// 马在棋盘内	
					*(pMove++)  = unsigned char( n );
			}
			*pMove = 0;
		}
	}
}

// 初始化象的着法预产生数组：
// 其值初始化为象移动的绝对位置。
// 不合法移动：塞象眼的移动，吃己方棋子的移动，必须检测。
void CPreMove::InitBishopMoves(unsigned char BishopMoves[][8])
{
	int i, m, n;
	unsigned char *pMove;
	int nBishopMoveDir[] = { -34, -30, 30, 34 };
	
	for(m=0x33; m<0xCC; m++)
	{
		if( nBoardIndex[m] )
		{
			pMove = BishopMoves[m];
			for(i=0; i<4; i++)
			{	
				n = m + nBishopMoveDir[i];
				if( nBoardIndex[n] && (m^n)<128 )			// (m^n)<128, 保证象不会过河
					*(pMove++)  = unsigned char( n );
			}
			*pMove = 0;
		}
	}
}


// 初始化士的着法预产生数组：
// 其值初始化为士移动的绝对位置。
// 不合法移动：吃己方棋子的移动，必须检测。
void CPreMove::InitGuardMoves(unsigned char GuardMoves[][8])
{
	int i, m, n;
	unsigned char *pMove;
	int nGuardMoveDir[] = { -17, -15, 15, 17 };

	for(m=0x36; m<0xC9; m++)
	{
		if( nCityIndex[m] )										// 在九宫内
		{
			pMove = GuardMoves[m];
			for(i=0; i<4; i++)
			{
				n = m + nGuardMoveDir[i];
				if( nCityIndex[n] )								// 在九宫内
					*(pMove++)  = unsigned char( n );
			}
			*pMove = 0;		// 表示不能再移动
		}
	}
}


// 初始化兵卒的着法预产生数组：
// 其值初始化为移动的绝对位置。
// 不合法移动：吃己方棋子的移动，必须检测。
void CPreMove::InitPawnMoves(unsigned char PawnMoves[][256][4])
{
	int i, m, n, Player;
	unsigned char *pMove;
	int PawnMoveDir[2][3] = { {16, -1, 1}, {-16, -1, 1} };							// 黑：下左右；  红：上左右

	for(Player=0; Player<=1; Player++)												//Player=0, 黑卒； Player=1, 红兵
	{
		for(m=0x33; m<0xCC; m++)
		{
			if( nBoardIndex[m] )
			{
				pMove = PawnMoves[Player][m];
				for(i=0; i<3; i++)
				{
					if( i>0 && ((!Player && m<128) || (Player && m>=128)) )			//未过河的兵不能左右晃动
						break;

					n = m + PawnMoveDir[Player][i];
					if( nBoardIndex[n] )										   //在棋盘范围内
						*(pMove++)  = unsigned char( n );
				}
				*pMove = 0;
			}
		}
	}	
}


// *****************************************************************************************************************
// *****************************************************************************************************************

// 车的位行与位列棋盘：包含吃子移动和非吃子移动
void CPreMove::InitBitRookMove( unsigned short xBitRookMove[][512], unsigned short yBitRookMove[][1024])
{
	int x, y, m, n, index;

	// 车的横向移动
	for(x=3; x<12; x++)
	{
		for(m=0; m<512; m++)
		{
			xBitRookMove[x][m] = 0;

			// 向左移动
			for(n=x-1; n>=3; n--)
			{
				index = 1<<(n-3);
				xBitRookMove[x][m] |= index;
				if( m & index )						// 遇到棋子，产生俘获
					break;
			}

			// 向右移动
			for(n=x+1; n<12; n++)
			{
				index = 1<<(n-3);
				xBitRookMove[x][m] |= index;
				if( m & index )						// 遇到棋子，产生俘获
					break;
			}
		}
	}

	// 车的纵向移动
	for(y=3; y<13; y++)
	{
		for(m=0; m<1024; m++)
		{
			yBitRookMove[y][m] = 0;

			// 向上移动
			for(n=y-1; n>=3; n--)
			{
				index = 1<<(n-3);
				yBitRookMove[y][m] |= index;
				if( m & index )						// 遇到棋子，产生俘获
					break;
			}

			// 向下移动
			for(n=y+1; n<13; n++)
			{
				index = 1<<(n-3);
				yBitRookMove[y][m] |= index;
				if( m & index )						// 遇到棋子，产生俘获
					break;
			}
		}
	}
}


// 炮的位行与位列棋盘：包含吃子移动和非吃子移动
void CPreMove::InitBitCannonMove( unsigned short xBitCannonMove[][512], unsigned short yBitCannonMove[][1024] )
{
	int x, y, m, n, index;

	// 炮的横向移动
	for(x=3; x<12; x++)
	{
		for(m=0; m<512; m++)
		{
			xBitCannonMove[x][m] = 0;

			// 向左移动
			for(n=x-1; n>=3; n--)				// 非吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到炮架子
				{
					n--;						// 跳跃跑架子
					break;
				}
				xBitCannonMove[x][m] |= index;
			}
			for( ; n>=3; n--)					// 吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到棋子，产生俘获
				{
					xBitCannonMove[x][m] |= index;
					break;
				}				
			}

			// 向右移动
			for(n=x+1; n<12; n++)				// 非吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到炮架子
				{
					n++;						// 跳跃跑架子
					break;
				}
				xBitCannonMove[x][m] |= index;
			}
			for( ; n<12; n++)					// 吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到棋子，产生俘获
				{
					xBitCannonMove[x][m] |= index;
					break;
				}				
			}
		}
	}

	// 炮的纵向移动
	for(y=3; y<13; y++)
	{
		for(m=0; m<1024; m++)
		{
			yBitCannonMove[y][m] = 0;

			// 向上移动
			for(n=y-1; n>=3; n--)				// 非吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到炮架子
				{
					n--;						// 跳跃跑架子
					break;
				}
				yBitCannonMove[y][m] |= index;
			}
			for( ; n>=3; n--)					// 吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到棋子，产生俘获
				{
					yBitCannonMove[y][m] |= index;
					break;
				}				
			}

			// 向下移动
			for(n=y+1; n<13; n++)				// 非吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到炮架子
				{
					n++;						// 跳跃跑架子
					break;
				}
				yBitCannonMove[y][m] |= index;
			}
			for( ; n<13; n++)					// 吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到棋子，产生俘获
				{
					yBitCannonMove[y][m] |= index;
					break;
				}				
			}
		}
	}
}

void CPreMove::InitBitSupperCannon( unsigned short xBitSupperCannon[][512], unsigned short yBitSupperCannon[][1024] )
{
	int x, y, m, n, index;

	// 炮的横向移动
	for(x=3; x<12; x++)
	{
		for(m=0; m<512; m++)
		{
			xBitSupperCannon[x][m] = 0;

			// 向左移动
			index = 0;
			for(n=x-1; n>=3; n--)				// 非吃子移动
			{
				if( m & (1<<(n-3)) )			// 遇到炮架子
				{
					index ++;
					if( index >= 2 )			// 跳跃两个炮架子
					{
						n --;
						break;
					}
				}
			}
			for( ; n>=3; n--)					// 吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到棋子，产生俘获
				{
					xBitSupperCannon[x][m] |= index;
					break;
				}				
			}

			// 向右移动
			index = 0;
			for(n=x+1; n<12; n++)				// 非吃子移动
			{
				if( m & (1<<(n-3)) )			// 遇到炮架子
				{
					index ++;
					if( index >= 2 )			// 跳跃两个炮架子
					{
						n ++;
						break;
					}
				}
			}
			for( ; n<12; n++)					// 吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到棋子，产生俘获
				{
					xBitSupperCannon[x][m] |= index;
					break;
				}				
			}
		}
	}

	// 炮的纵向移动
	for(y=3; y<13; y++)
	{
		for(m=0; m<1024; m++)
		{
			yBitSupperCannon[y][m] = 0;

			// 向上移动
			index = 0;
			for(n=y-1; n>=3; n--)				// 非吃子移动
			{
				if( m & (1<<(n-3)) )			// 遇到炮架子
				{
					index ++;
					if( index >= 2 )			// 跳跃两个炮架子
					{
						n --;
						break;
					}
				}
			}
			for( ; n>=3; n--)					// 吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到棋子，产生俘获
				{
					yBitSupperCannon[y][m] |= index;
					break;
				}				
			}

			// 向下移动
			index = 0;
			for(n=y+1; n<13; n++)				// 非吃子移动
			{
				if( m & (1<<(n-3)) )			// 遇到炮架子
				{
					index ++;
					if( index >= 2 )			// 跳跃两个炮架子
					{
						n ++;
						break;
					}
				}
			}
			for( ; n<13; n++)					// 吃子移动
			{
				index = 1<<(n-3);
				if( m & index )					// 遇到棋子，产生俘获
				{
					yBitSupperCannon[y][m] |= index;
					break;
				}				
			}
		}
	}
}