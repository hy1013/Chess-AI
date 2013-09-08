////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 源文件：Search.cpp                                                                                     //
// *******************************************************************************************************//
// 中国象棋通用引擎----简单测试引擎示例，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： IwfWcf                                                                                       //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 作为CMoveGen的子类，继承父类的数据，如棋盘、棋子、着法等。                                          //
// 2. 接收界面的数据，初始化为搜索需要的信息。                                                            //
// 3. 调用CHashTable类，执行和撤销着法。                                                                  //
// 4. 采用冒泡法对着法排序                                                                                //
// 5. 主控搜索函数 MainSearch()                                                                           //
// 6.                                                                         //
// 7. 博弈树搜索算法 AlphaBetaSearch()                                                                    //
// 8.                                                                     //
// 9. 基于1999年版《中国象棋竞赛规则》实现循环检测                                                        //
//10. 循环的返回值问题、与Hash表冲突问题。                            //
//11. 利用Hash表获取主分支                                                                                //
//12. 搜索时间的控制                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <time.h>
#include <string.h>
#include <assert.h>

#include "ucci.h"
#include "Search.h"

static const unsigned int nAdaptive_R = 3;
static const unsigned int nVerified_R = 3;
static const unsigned int BusyCounterMask = 4095;
static const unsigned int BitAttackPieces = 0xF87EF87E;    //11111000011111101111100001111110 兵兵兵兵兵士士象象马马炮炮车车将卒卒卒卒卒士士相相马马炮炮车车帅


CSearch::CSearch(void)
{	
	// 初始化空着
	StepRecords[0] = 0;	
	
	bPruning = 1 ;							// 允许使用NullMove
	nSelectivity = 0;						// 选择性系数，通常有0,1,2,3四个级别
	Debug = 0;								// bDegug = 0, 输出简明扼要的搜索信息
	SelectMask = 0;
	nStyle = 1;
	Ponder = 0;
	bStopThinking = 0;
	QhMode = 0;
	bBatch = 0;
	StyleShift = 5;
	bUseOpeningBook = 0;
	NaturalBouts = 120;
	nBanMoveNum = 0;

	// 初始化Hash表，分配21+1=22级Hash表，64MB
	m_Hash.NewHashTable(22, 12);
	m_Hash.ClearHashTable();

	// 初始化历史表
	UpdateHistoryRecord( 0 );
}

CSearch::~CSearch(void)
{
	m_Hash.DeleteHashTable();
}

// 判断移动是否为禁着
int CSearch::IsBanMove(CChessMove move)
{
	int m;
	for(m=0; m<nBanMoveNum; m++)
	{
		if( move == BanMoveList[m] )
			return true;
	}
	return 0;
}

//深度迭代搜索
int CSearch::MainSearch(int nDepth, long nProperTimer, long nLimitTimer)
{
	// 初始化一些有用的变量
	int w, MoveStr, score=0;
	nPvLineNum = PvLine[0] = 0;
	
	// 这些变量用于测试搜索树性能的各种参数
	nNullMoveNodes = nNullMoveCuts = 0;
	nHashMoves = nHashCuts = 0;	
	nAlphaNodes = nPvNodes = nBetaNodes = 0;
	nTreeNodes = nLeafNodes = nQuiescNodes = 0;
	m_Hash.nCollision = m_Hash.nHashAlpha = m_Hash.nHashExact = m_Hash.nHashBeta = 0;
	nTreeHashHit = nLeafHashHit = 0;
	nCheckEvasions = 0;
	nZugzwang = 0;	
	nCapCuts = nCapMoves = 0;
	for(w=0; w<MaxKiller; w++)
		nKillerCuts[w] = nKillerNodes[w] = 0;
	nCheckCounts = nNonCheckCounts = 0;
	// 这些变量用于测试搜索树性能的各种参数


	//一、分配搜索时间
	StartTimer = clock();
	//nMinTimer = StartTimer + unsigned int(nProperTimer*0.618f);
	nMinTimer = StartTimer + CLOCKS_PER_SEC * 19.5;
	nMaxTimer = unsigned int(nProperTimer*1.618f);
	if(nMaxTimer > nLimitTimer)
		nMaxTimer = nLimitTimer;
	nMaxTimer += StartTimer;
	bStopThinking = 0;


	//二、输出当前局面
	fen.BoardToFen(Board, Player, nNonCapNum, nCurrentStep, StepRecords);
	fprintf(OutFile, "info BoardToFen: %s\n", fen.FenStr);
	fflush(OutFile);


	//三、在开局库中进行搜索
	//CChessMove BookMove;
	//if(bUseOpeningBook && m_Hash.ProbeOpeningBook(BookMove, Player))
	//{
	//	nPvLineNum = 1;
	//	PvLine[0] = BookMove;
	//	score = BookMove >> 16;
	//	MoveStr = Coord( BookMove );

	//	fprintf(OutFile, "info depth 0 score %d pv %.4s\n", score, &MoveStr);
	//	fflush(OutFile);
	//	fprintf(OutFile, "bestmove %.4s\n", &MoveStr);
	//	fflush(OutFile);

	//	return score;
	//}


	nStartStep = nCurrentStep;		// 开始搜索时的半回合数。

	score = SimpleSearch(nDepth, -WINSCORE, WINSCORE);
		

	//返回搜索结果
	// 若有合法的移动，输出 bestmove %.4s 和 ponder %.4s  以及详细的搜索信息。
	PopupInfo(MaxDepth, score, 1);
	if( nPvLineNum )
	{
		MoveStr = Coord(PvLine[0]);
		fprintf(OutFile, "bestmove %.4s", &MoveStr);
		if(Ponder && nPvLineNum>1)
		{
			MoveStr = Coord(PvLine[1]);
			fprintf(OutFile, " ponder %.4s", &MoveStr);
		}
		fprintf(OutFile, "\n");
		fflush(OutFile);
	}
	// 出现循环，不存在合法的移动，返回score。意味着结束游戏。
	else
	{
		fprintf(OutFile, "depth %d score %d\n", MaxDepth, score);
		fflush(OutFile);
		fprintf(OutFile, "nobestmove\n");
		fflush(OutFile);
	}
	//fprintf(OutFile, "\n\n");
	//fflush(OutFile);


	//清除Hash表和历史启发表
	StartTimer = clock() - StartTimer;
	m_Hash.ClearHashTable( 2 );
	SaveMoves("SearchInfo.txt");	
	UpdateHistoryRecord( 4 );
	nBanMoveNum = 0;

	return(score);
}

// 第一层的穷举搜索
int CSearch::SimpleSearch(int depth, int alpha, int beta)
{	
	//nTreeNodes ++;	// 统计树枝节点

	int score, nBestValue = -WINSCORE, w, way, nCaptured;	
		
	const int  ply = nCurrentStep - nStartStep;			                    // 获取当前的回合数
	const unsigned int nNonCaptured = nNonCapNum;
	const int nChecked = (StepRecords[nCurrentStep-1] & 0xFF000000) >> 24;	// 从走法队列(全局变量)中获取(我方)将军标志
	
	CChessMove ThisMove, BestMove = 0;									// 初始化为空着。若形成困闭，可以返回此空着。
	CChessMove HashMove = 0;												// HashMove

	int HashFlag = HashAlpha;												// Hash标志										// 搜索窗口返回的极小极大值
	nFirstLayerMoves = 0;

	for(w=1; w<=depth; w++)
	{	
		MaxDepth = w;
		UpdateHistoryRecord( 0 );

		PII temp = PrincipalVariation(MaxDepth, alpha, beta);
		if (temp.X == -WINSCORE && temp.Y == -WINSCORE) break;
		nBestValue = temp.X, BestMove = temp.Y;

		// 若強行終止思考，停止搜索
		if(bStopThinking)
			break;

		//if (clock() > nMinTimer) break;

		// 在規定的深度內，遇到殺棋，停止思考。
		if(nBestValue<-MATEVALUE || nBestValue>MATEVALUE)
			break;
	}

	m_Hash.RecordHash( BestMove, nBestValue, HashFlag, 0, ply, Player );
	return nBestValue;
}

PII CSearch::PrincipalVariation(int depth, int alpha, int beta)
{
	int score, nBestValue = -WINSCORE, w, way, nCaptured;	
		
	const int  ply = nCurrentStep - nStartStep;			                    // 获取当前的回合数
	const unsigned int nNonCaptured = nNonCapNum;
	const int nChecked = (StepRecords[nCurrentStep-1] & 0xFF000000) >> 24;	// 从走法队列(全局变量)中获取(我方)将军标志
	
	CChessMove ThisMove, BestMove = 0;									// 初始化为空着。若形成困闭，可以返回此空着。
	CChessMove HashMove = 0;												// HashMove

	int HashFlag = HashAlpha;												// Hash标志										// 搜索窗口返回的极小极大值
	nFirstLayerMoves = 0;

	if (!depth) return MP(m_Evalue.Evaluation(Player), BestMove);

	nTreeNodes ++;	// 统计树枝节点

	//产生所有合法的移动
	//1.将军局面  ：产生将军逃避着法；
	//2.非将军局面：吃子着法和非吃子着法，吃子着法附加历史得分，全部按历史启发处理。
	CChessMove ChessMove[111], tmp[111];
	if( nChecked )
		way = CheckEvasionGen(Player, nChecked, ChessMove);					// 产生逃避将军的着法
	else
	{
		way  = CapMoveGen(Player, ChessMove);								// 产生所有的吃子移动
		//for(w=0; w<way; w++)
			//ChessMove[w] += HistoryRecord[ChessMove[w] & 0xFFFF] << 16;		// 吃子着法 + 历史启发
		way += MoveGenerator(Player, ChessMove+way);						// 产生所有非吃子移动
	}

	for (int i = 0; i < way; i++) tmp[i] = ChessMove[i];
	if (MaxDepth > 1 && depth == MaxDepth) {
		for (int i = 0; i < way; i++) ChessMove[i] = tmp[Record[way - 1 - i].Y];
	} else {
		for (int i = 0; i < way; i++) Score[i].X = HistoryRecord[ChessMove[i] & 0xFFFF], Score[i].Y = i;
		sort(Score, Score + way);
		for (int i = 0; i < way; i++) ChessMove[i] = tmp[Score[way - 1 - i].Y];
	}

	if (depth == MaxDepth) 
		for (int i = 0; i < way; i++) Record[i].X = -WINSCORE, Record[i].Y = i;
	
	int nChecking;
	for(w=0; w<way; w++)
	{
		ThisMove = ChessMove[w] & 0xFFFF;

		// 过滤HashMove和禁止着法
		if( /*ThisMove==HashMove ||*/ IsBanMove(ThisMove) )
			continue;
		
		assert(Board[ThisMove & 0xFF]!=16 && Board[ThisMove & 0xFF]!=32);		// 不应该出现，否则有错误，通过将军检测，提前一步过滤调这类走法
		nCaptured = MovePiece( ThisMove );										// 注意：移动后Player已经表示对方，下面的判断不要出错。尽管这样很别扭，但在其它地方很方便，根本不用管了
		
		//if (Checked(1 - Player)) nBestValue = ply+1-WINSCORE;
		nBestValue = -PrincipalVariation(depth - 1, -beta, -alpha).X;
		if (clock() > nMinTimer) return(MP(-WINSCORE, -WINSCORE));
		if (depth == MaxDepth) Record[w].X = nBestValue;
		BestMove = ThisMove;
		UndoMove();					// 恢复移动，恢复移动方，恢复一切
		nNonCapNum = nNonCaptured;	// 恢复原来的无杀子棋步数目
		break;
	}

	for(++w; w<way; w++)
	{
		ThisMove = ChessMove[w] & 0xFFFF;

		// 过滤HashMove和禁止着法
		if( /*ThisMove==HashMove || */IsBanMove(ThisMove) )
			continue;
		if (nBestValue < beta) {
			if (nBestValue > alpha) alpha = nBestValue;
			nCaptured = MovePiece( ThisMove );
			if (Checked(1 - Player)) score = ply+1-WINSCORE;
			else {
				score = -PrincipalVariation(depth - 1, -alpha - 1, -alpha).X;
				if (clock() > nMinTimer) return(MP(-WINSCORE, -WINSCORE));
				if (score > alpha && score < beta) {
					nBestValue = -PrincipalVariation(depth - 1, -beta, -score).X;
					if (clock() > nMinTimer) return(MP(-WINSCORE, -WINSCORE));
					BestMove = ThisMove;
					if (depth == MaxDepth) Record[w].X = nBestValue;
				} else {
					if (score > nBestValue) nBestValue = score, BestMove = ThisMove;
					if (depth == MaxDepth) Record[w].X = score;
				}
			}
			UndoMove();					// 恢复移动，恢复移动方，恢复一切
			nNonCapNum = nNonCaptured;	// 恢复原来的无杀子棋步数目
		} else
			break;
	}

	HistoryRecord[BestMove] += 2 << depth;

	if (depth == MaxDepth) sort(Record, Record + way);

	return MP(nBestValue, BestMove);
}

PII CSearch::NegaScout(int depth, int alpha, int beta)
{
	int score, nBestValue = -WINSCORE, w, way, nCaptured;	
		
	const int  ply = nCurrentStep - nStartStep;			                    // 获取当前的回合数
	const unsigned int nNonCaptured = nNonCapNum;
	const int nChecked = (StepRecords[nCurrentStep-1] & 0xFF000000) >> 24;	// 从走法队列(全局变量)中获取(我方)将军标志
	
	CChessMove ThisMove, BestMove = 0;									// 初始化为空着。若形成困闭，可以返回此空着。
	CChessMove HashMove = 0;												// HashMove

	int HashFlag = HashAlpha;												// Hash标志										// 搜索窗口返回的极小极大值
	nFirstLayerMoves = 0;

	if (!depth) return MP(m_Evalue.Evaluation(Player), BestMove);

	nTreeNodes ++;	// 统计树枝节点

	//产生所有合法的移动
	//1.将军局面  ：产生将军逃避着法；
	//2.非将军局面：吃子着法和非吃子着法，吃子着法附加历史得分，全部按历史启发处理。
	CChessMove ChessMove[111], tmp[111];
	if( nChecked )
		way = CheckEvasionGen(Player, nChecked, ChessMove);					// 产生逃避将军的着法
	else
	{
		way  = CapMoveGen(Player, ChessMove);								// 产生所有的吃子移动
		//for(w=0; w<way; w++)
			//ChessMove[w] += HistoryRecord[ChessMove[w] & 0xFFFF] << 16;		// 吃子着法 + 历史启发
		way += MoveGenerator(Player, ChessMove+way);						// 产生所有非吃子移动
	}

	//if (MaxDepth > 1) {
		for (int i = 0; i < way; i++) tmp[i] = ChessMove[i];
		if (depth == MaxDepth) {
			for (int i = 0; i < way; i++) ChessMove[i] = tmp[Record[way - 1 - i].Y];
		} else {
			for (int i = 0; i < way; i++) Score[i].X = HistoryRecord[ChessMove[i] & 0xFFFF], Score[i].Y = i;
			sort(Score, Score + way);
			for (int i = 0; i < way; i++) ChessMove[i] = tmp[Score[way - 1 - i].Y];
		}
	//}

	if (depth == MaxDepth) 
		for (int i = 0; i < way; i++) Record[i].X = -WINSCORE, Record[i].Y = i;
	
	int nChecking, a = alpha, b = beta;
	bool first = true;
	for(w=0; w<way; w++)
	{
		ThisMove = ChessMove[w] & 0xFFFF;

		// 过滤HashMove和禁止着法
		if( /*ThisMove==HashMove ||*/ IsBanMove(ThisMove) )
			continue;
		
		assert(Board[ThisMove & 0xFF]!=16 && Board[ThisMove & 0xFF]!=32);		// 不应该出现，否则有错误，通过将军检测，提前一步过滤调这类走法
		nCaptured = MovePiece( ThisMove );										// 注意：移动后Player已经表示对方，下面的判断不要出错。尽管这样很别扭，但在其它地方很方便，根本不用管了
		
		if (Checked(1 - Player)) score = ply+1-WINSCORE;
		else {
			score = -NegaScout(depth - 1, -b, -a).X;
			if (clock() > nMinTimer) return(MP(-WINSCORE, -WINSCORE));
			if (score > alpha && score < beta && !first) {
				a = -NegaScout(depth - 1, -beta, -score).X;
				if (clock() > nMinTimer) return(MP(-WINSCORE, -WINSCORE));
				BestMove = ThisMove;
				if (depth == MaxDepth) Record[w].X = a;
			}
			UndoMove();					// 恢复移动，恢复移动方，恢复一切
			nNonCapNum = nNonCaptured;	// 恢复原来的无杀子棋步数目
			first = false;
			if (depth == MaxDepth) Record[w].X = score;
			if (score > a) a = score, BestMove = ThisMove;
			if (a >= beta) break;
			b = a + 1;
		}
	}

	HistoryRecord[BestMove] += 2 << depth;

	if (depth == MaxDepth) sort(Record, Record + way);

	return MP(a, BestMove);
}

PII CSearch::FAlphaBeta(int depth, int alpha, int beta)
{
	nTreeNodes ++;	// 统计树枝节点

	int score, current = -WINSCORE, w, way, nCaptured;	
		
	const int  ply = nCurrentStep - nStartStep;			                    // 获取当前的回合数
	const unsigned int nNonCaptured = nNonCapNum;
	const int nChecked = (StepRecords[nCurrentStep-1] & 0xFF000000) >> 24;	// 从走法队列(全局变量)中获取(我方)将军标志
	
	CChessMove ThisMove, BestMove = 0;									// 初始化为空着。若形成困闭，可以返回此空着。
	CChessMove HashMove = 0;												// HashMove

	int HashFlag = HashAlpha;												// Hash标志										// 搜索窗口返回的极小极大值
	nFirstLayerMoves = 0;

	if (!depth) return MP(m_Evalue.Evaluation(Player), BestMove);

	//int nHashValue = m_Hash.ProbeHash(HashMove, alpha, beta, 127, ply, Player);
	
	//产生所有合法的移动
	//1.将军局面  ：产生将军逃避着法；
	//2.非将军局面：吃子着法和非吃子着法，吃子着法附加历史得分，全部按历史启发处理。
	CChessMove ChessMove[111];
	if( nChecked )
		way = CheckEvasionGen(Player, nChecked, ChessMove);					// 产生逃避将军的着法
	else
	{
		way  = CapMoveGen(Player, ChessMove);								// 产生所有的吃子移动
		for(w=0; w<way; w++)
			ChessMove[w] += HistoryRecord[ChessMove[w] & 0xFFFF] << 16;		// 吃子着法 + 历史启发
		way += MoveGenerator(Player, ChessMove+way);						// 产生所有非吃子移动
	}
	
	int nChecking;
	for(w=0; w<way; w++)
	{
		ThisMove = ChessMove[w] & 0xFFFF;

		// 过滤HashMove和禁止着法
		if( ThisMove==HashMove || IsBanMove(ThisMove) )
			continue;
		
		assert(Board[ThisMove & 0xFF]!=16 && Board[ThisMove & 0xFF]!=32);		// 不应该出现，否则有错误，通过将军检测，提前一步过滤调这类走法
		nCaptured = MovePiece( ThisMove );										// 注意：移动后Player已经表示对方，下面的判断不要出错。尽管这样很别扭，但在其它地方很方便，根本不用管了
	
		score = -FAlphaBeta(depth - 1, -beta, -alpha).X;
		UndoMove();					// 恢复移动，恢复移动方，恢复一切
		nNonCapNum = nNonCaptured;	// 恢复原来的无杀子棋步数目
		if(score > current) {
			current = score, BestMove = ThisMove;
			if (score > alpha) alpha = score;
			if (score >= beta) break;
		}
	}
	
	//m_Hash.RecordHash(BestMove, current, HashFlag, depth, ply, Player);
	return MP(current, BestMove);
}

int CSearch::MovePiece(const CChessMove move)
{	
	int nSrc = (move & 0xFF00) >> 8;
	int nDst = move & 0xFF;
	int nMovedChs = Board[nSrc];
	int nCaptured = Board[nDst];
	int nMovedPiece = nPieceType[nMovedChs];

	assert( nMovedChs>=16 && nMovedChs<48 );
	assert( nCaptured>=0 && nCaptured<48 );
	

	//更新棋盘
	Board[nSrc] = 0;
	Board[nDst] = nMovedChs;


	m_Hash.ZobristKey  ^= m_Hash.ZobristKeyTable[nMovedPiece][nSrc] ^ m_Hash.ZobristKeyTable[nMovedPiece][nDst];
	m_Hash.ZobristLock ^= m_Hash.ZobristLockTable[nMovedPiece][nSrc] ^ m_Hash.ZobristLockTable[nMovedPiece][nDst];

	
	//更新棋子坐标
	Piece[nMovedChs] = nDst;
	Evalue[Player] += PositionValue[nMovedPiece][nDst] - PositionValue[nMovedPiece][nSrc];	// 更新估值
	

	if( nCaptured )
	{
		nNonCapNum = 0;
		Piece[nCaptured] = 0;
		BitPieces ^= 1<<(nCaptured-16);

		int nKilledPiece = nPieceType[nCaptured];
		
		m_Hash.ZobristKey  ^= m_Hash.ZobristKeyTable[nKilledPiece][nDst];
		m_Hash.ZobristLock ^= m_Hash.ZobristLockTable[nKilledPiece][nDst];

		Evalue[1-Player] -= PositionValue[nKilledPiece][nDst] + BasicValues[nKilledPiece];
	}
	else
		nNonCapNum ++;				// 双方无杀子的半回合数


	// 擦除起点nSrc的位行与位列，置“0”
	xBitBoard[ nSrc >> 4 ]  ^= xBitMask[nSrc];
	yBitBoard[ nSrc & 0xF ] ^= yBitMask[nSrc];	

	// 更新终点nDst的位行与位列，置“1”
	xBitBoard[ nDst >> 4 ]  |= xBitMask[nDst];
	yBitBoard[ nDst & 0xF ] |= yBitMask[nDst];

	
	//记录当前局面的ZobristKey，用于循环探测、将军检测
	StepRecords[nCurrentStep] = move  | (nCaptured<<16);
	nZobristBoard[nCurrentStep] = m_Hash.ZobristKey;		// 当前局面的索引
	nCurrentStep++;


	Player = 1 - Player;		// 改变移动方
	//m_Hash.ZobristKey ^= m_Hash.ZobristKeyPlayer;
    //m_Hash.ZobristLock ^= m_Hash.ZobristLockPlayer;
	
	return(nCaptured);
}

void CSearch::UndoMove(void)
{
	CChessMove move = StepRecords[nCurrentStep-1];
	int nSrc = (move & 0xFF00) >> 8;;
	int nDst = move & 0xFF;
	int nMovedChs = Board[nDst];
	int nMovedPiece = nPieceType[nMovedChs];
	int nCaptured = (move & 0xFF0000) >> 16;


	// 首先恢复移动方
	Player = 1 - Player;		
	//m_Hash.ZobristKey ^= m_Hash.ZobristKeyPlayer;
    //m_Hash.ZobristLock ^= m_Hash.ZobristLockPlayer;

	m_Hash.ZobristKey  ^= m_Hash.ZobristKeyTable[nMovedPiece][nSrc] ^ m_Hash.ZobristKeyTable[nMovedPiece][nDst];
	m_Hash.ZobristLock ^= m_Hash.ZobristLockTable[nMovedPiece][nSrc] ^ m_Hash.ZobristLockTable[nMovedPiece][nDst];

	//更新棋盘与棋子
	Board[nSrc] = nMovedChs;
	Board[nDst] = nCaptured;
	Piece[nMovedChs] = nSrc;
	Evalue[Player] -= PositionValue[nMovedPiece][nDst] - PositionValue[nMovedPiece][nSrc];	// 更新估值


	// 恢复位行与位列的起始位置nSrc，使用“|”操作符置“1”
	xBitBoard[ nSrc >> 4 ]  |= xBitMask[nSrc];
	yBitBoard[ nSrc & 0xF ] |= yBitMask[nSrc];
	
	if( nCaptured )							//吃子移动
	{
		int nKilledPiece = nPieceType[nCaptured];

		Piece[nCaptured] = nDst;			//恢复被杀棋子的位置。
		BitPieces ^= 1<<(nCaptured-16);

		m_Hash.ZobristKey  ^= m_Hash.ZobristKeyTable[nKilledPiece][nDst];
		m_Hash.ZobristLock ^= m_Hash.ZobristLockTable[nKilledPiece][nDst];

		Evalue[1-Player] += PositionValue[nKilledPiece][nDst] + BasicValues[nKilledPiece];
	}
	else									//若是非吃子移动，必须把俘获位置置"0"
	{
		// 清除位行与位列的起始位置nDst，使用“^”操作符置“0”
		// 反之，若是吃子移动，终点位置本来就是"1"，所以不用恢复。
		xBitBoard[ nDst >> 4 ]  ^= xBitMask[nDst];
		yBitBoard[ nDst & 0xF ] ^= yBitMask[nDst];
	}

	//清除走法队列
	nCurrentStep --;
	nNonCapNum --;
}

// 根据棋子位置信息，初始化所有棋盘与棋子的数据
// 也可使用棋盘信息，需循环256次，速度稍慢。
void CSearch::InitBitBoard(const int Player, const int nCurrentStep)
{
	int m,n,x,y;	
	int chess;
	
	// 初始化，清零
	BitPieces = 0;
	Evalue[0] = Evalue[1] = 0;
	m_Hash.ZobristKey  = 0;
	m_Hash.ZobristLock = 0;
	for(x=0; x<16; x++)
		xBitBoard[x] = 0;
	for(y=0; y<16; y++)
		yBitBoard[y] = 0;
	
	// 根据32颗棋子位置更新
	for(n=16; n<48; n++)
	{
		m = Piece[n];													// 棋子位置
		if( m )															// 在棋盘上
		{
			chess               = nPieceType[n];						// 棋子类型：0～14
			BitPieces          |= 1<<(n-16);							// 32颗棋子位图
			xBitBoard[m >> 4 ] |= xBitMask[m];							// 位行
			yBitBoard[m & 0xF] |= yBitMask[m];							// 位列
			m_Hash.ZobristKey  ^= m_Hash.ZobristKeyTable[chess][m];		// Hash键
			m_Hash.ZobristLock ^= m_Hash.ZobristLockTable[chess][m];	// Hash锁
			
			if(n!=16 && n!=32)			
				Evalue[ (n-16)>>4 ] += PositionValue[chess][m] + BasicValues[chess];
		}
	}

	//m_Hash.InitZobristPiecesOnBoard( Piece );

	// 用于循环检测
	nZobristBoard[nCurrentStep-1] =  m_Hash.ZobristKey;

	// 当前移动方是否被将军，写入走法队列
	// StepRecords[nCurrentStep-1] |= Checking(Player) << 24;
}

//还应参照棋规作更详细的判断，为了通用，最好不使用CString类
char *CSearch::GetStepName(CChessMove ChessMove, int *Board) const
{
	//棋子编号
	static char StepName[12];	// 必须用静态变量，否则不能返回

	static const char ChessName[14][4] = {"帥","車","炮","马","象","士","卒", "將","車","炮","馬","相","仕","兵"};

	static const char PostionName[2][16][4] = { {"", "", "", "１","２","３","４","５","６","７","８","９", "", "", "", ""}, 
	                                            {"", "", "", "九","八","七","六","五","四","三","二","一", "", "", "", ""} };

	const int nSrc = (ChessMove & 0xFF00) >> 8;
	const int nDst = ChessMove & 0xFF;

	if( !ChessMove )
		return("HashMove");

	const int nMovedChs = Board[nSrc];
	const int x0 = nSrc & 0xF;
	const int y0 = nSrc >> 4;
	const int x1 = nDst & 0xF;
	const int y1 = nDst >> 4;

	const int Player = (nMovedChs-16) >> 4;	

	strcpy( StepName, ChessName[nPieceType[nMovedChs]] );
	strcat( StepName, PostionName[Player][x0] );
	
	//检查此列x0是否存在另一颗成对的棋子.
	int y,chess;
	if( nPieceID[nMovedChs]!=0 && nPieceID[nMovedChs]!=4 && nPieceID[nMovedChs]!=5 )	// 将、士、象不用区分
	{
		for(y=3; y<13; y++)
		{
			chess = Board[ (y<<4) | x0 ];

			if( !chess || y==y0)														// 无子或者同一颗棋子，继续搜索
				continue;

			if( nPieceType[nMovedChs] == nPieceType[chess] )							// 遇到另一个相同的棋子
			{
				if( !Player )			// 黑子
				{
					if(y > y0)
						strcpy( StepName, "前" );
					else
						strcpy( StepName, "后" );
				}
				else					// 红子
				{
					if(y < y0)
						strcpy( StepName, "前" );
					else
						strcpy( StepName, "后" );
				}

				strcat( StepName, ChessName[nPieceType[nMovedChs]] );
				break;
			}
		}
	}

	int piece = nPieceID[nMovedChs];

	//进, 退, 平
	if(y0==y1)
	{
		strcat( StepName, "平" );
		strcat( StepName, PostionName[Player][x1]);					// 平，任何棋子都以绝对位置表示
	}
	else if((!Player && y1>y0) || (Player && y1<y0))
	{
		strcat( StepName, "进" );

		if(piece==3 || piece==4 || piece==5)						// 马、象、士用绝对位置表示
			strcat( StepName, PostionName[Player][x1] );			
		else if(Player)												// 将、车、炮、兵用相对位置表示
			strcat( StepName, PostionName[1][y1-y0+12] );			// 红方
		else
			strcat( StepName, PostionName[0][y1-y0+2] );			// 黑方
	}
	else
	{
		strcat( StepName, "退" );

		if(piece==3 || piece==4 || piece==5)						// 马、象、士用绝对位置表示
			strcat( StepName, PostionName[Player][x1] );			
		else if(Player)												// 将、车、炮、兵用相对位置表示
			strcat( StepName, PostionName[1][y0-y1+12] );			// 红方
		else
			strcat( StepName, PostionName[0][y0-y1+2] );			// 黑方		
	}

	return(StepName);
}



// 获得主分支
// 由于使用了Hash表，有时主分支是错误的，有待修正！？？？
void CSearch::GetPvLine(void)
{
	CHashRecord *pHashIndex = m_Hash.pHashList[Player] + (m_Hash.ZobristKey & m_Hash.nHashMask);		//找到当前棋盘Zobrist对应的Hash表的地址

	if((pHashIndex->flag & HashExist) && pHashIndex->zobristlock==m_Hash.ZobristLock)
	{
		if( pHashIndex->move )
		{
			PvLine[nPvLineNum] = pHashIndex->move;
			
			MovePiece( PvLine[nPvLineNum] );

			nPvLineNum++;

			if( nNonCapNum<4 || !RepetitionDetect() )
				GetPvLine();

			UndoMove();
		}
	}
}

void CSearch::PopupInfo(int depth, int score, int Debug)
{
	unsigned int n;
	int MoveStr;
	if(depth)
	{
		fprintf(OutFile, "info depth %d score %d pv", depth, score);
		
		n = nNonCapNum;
		nPvLineNum = 0;
		GetPvLine();
		nNonCapNum = n;

		for(n=0; n<nPvLineNum; n++) 
		{
			MoveStr = Coord(PvLine[n]);
			fprintf(OutFile, " %.4s", &MoveStr);
		}

		fprintf(OutFile, "\n");
		fflush(OutFile);
	}

	if(Debug)
	{
		n = nTreeNodes + nLeafNodes + nQuiescNodes;
		fprintf(OutFile, "info Nodes %d = %d(T) + %d(L) + %d(Q)\n", n, nTreeNodes, nLeafNodes, nQuiescNodes);
		fflush(OutFile);

		float SearchTime = (clock() - StartTimer)/(float)CLOCKS_PER_SEC;
		fprintf(OutFile, "info TimeSpan = %.3f s\n", SearchTime);
		fflush(OutFile);

		fprintf(OutFile, "info NPS = %d\n", int(n/SearchTime));
		fflush(OutFile);
	}	
}


void CSearch::SaveMoves(char *szFileName)
{
	unsigned int m, n;
	int k, nSrc, nDst, nCaptured;
	
	// 创建文件，并删除原来的内容。利用这种格式化输出比较方便。
	FILE *out = fopen(szFileName, "w+");

	fprintf(out, "***************************搜索信息***************************\n\n");

	fprintf(out, "搜索深度：%d\n", MaxDepth);
	n = nTreeNodes + nLeafNodes + nQuiescNodes;
	fprintf(out, "TreeNodes : %u\n", n);
	fprintf(out, "TreeStruct: BranchNodes = %10u\n", nTreeNodes);
	fprintf(out, "            LeafNodes   = %10u\n", nLeafNodes);
	fprintf(out, "            QuiescNodes = %10u\n\n", nQuiescNodes);

	float TimeSpan = StartTimer/1000.0f;
	fprintf(out, "搜索时间    :   %8.3f 秒\n", TimeSpan);
	fprintf(out, "枝叶搜索速度:   %8.0f NPS\n", (nTreeNodes+nLeafNodes)/TimeSpan);
	fprintf(out, "整体搜索速度:   %8.0f NPS\n\n", n/TimeSpan);

	fprintf(out, "Hash表大小: %d Bytes  =  %d M\n", m_Hash.nHashSize*2*sizeof(CHashRecord), m_Hash.nHashSize*2*sizeof(CHashRecord)/1024/1024);
	fprintf(out, "Hash覆盖率: %d / %d = %.2f%%\n\n", m_Hash.nHashCovers, m_Hash.nHashSize*2, m_Hash.nHashCovers/float(m_Hash.nHashSize*2.0f)*100.0f);

	unsigned int nHashHits = m_Hash.nHashAlpha+m_Hash.nHashExact+m_Hash.nHashBeta;
	fprintf(out, "Hash命中: %d = %d(alpha:%.2f%%) + %d(exact:%.2f%%) +%d(beta:%.2f%%)\n", nHashHits, m_Hash.nHashAlpha, m_Hash.nHashAlpha/(float)nHashHits*100.0f, m_Hash.nHashExact, m_Hash.nHashExact/(float)nHashHits*100.0f, m_Hash.nHashBeta, m_Hash.nHashBeta/(float)nHashHits*100.0f);
	fprintf(out, "命中概率: %.2f%%\n", nHashHits/float(nTreeNodes+nLeafNodes)*100.0f);
	fprintf(out, "树枝命中: %d / %d = %.2f%%\n", nTreeHashHit, nTreeNodes, nTreeHashHit/(float)nTreeNodes*100.0f);
	fprintf(out, "叶子命中: %d / %d = %.2f%%\n\n", nLeafHashHit, nLeafNodes, nLeafHashHit/(float)nLeafNodes*100.0f);

	fprintf(out, "NullMoveCuts   = %u\n", nNullMoveCuts);
	fprintf(out, "NullMoveNodes  = %u\n", nNullMoveNodes);
	fprintf(out, "NullMove剪枝率 = %.2f%%\n\n", nNullMoveCuts/(float)nNullMoveNodes*100.0f);

	fprintf(out, "Hash冲突   : %d\n", m_Hash.nCollision);
	fprintf(out, "Null&Kill  : %d\n", m_Hash.nCollision-nHashMoves);
	fprintf(out, "HashMoves  : %d\n", nHashMoves);
	fprintf(out, "HashCuts   : %d\n", nHashCuts);
	fprintf(out, "Hash剪枝率 : %.2f%%\n\n", nHashCuts/(float)nHashMoves*100.0f);

	fprintf(out, "杀手移动 : \n");
	k = n = 0;
	for(m=0; m<MaxKiller; m++)
	{
		fprintf(out, "    Killer   %d : %8d /%8d = %.2f%%\n", m+1, nKillerCuts[m], nKillerNodes[m], nKillerCuts[m]/float(nKillerNodes[m]+0.001f)*100.0f);
		n += nKillerCuts[m];
		k += nKillerNodes[m];
	}
	fprintf(out, "    杀手剪枝率 : %8d /%8d = %.2f%%\n\n", n, k, n/float(k+0.001f)*100.0f);


	fprintf(out, "吃子移动剪枝率 = %d / %d = %.2f%%\n\n", nCapCuts, nCapMoves, nCapCuts/(float)nCapMoves*100.0f);


	m = nBetaNodes + nPvNodes + nAlphaNodes;
	fprintf(out, "非吃子移动: %d\n", m);	
	fprintf(out, "    BetaNodes: %10d  %4.2f%%\n", nBetaNodes, nBetaNodes/float(m)*100.0f);
	fprintf(out, "    PvNodes  : %10d  %4.2f%%\n", nPvNodes, nPvNodes/float(m)*100.0f);
	fprintf(out, "    AlphaNode: %10d  %4.2f%%\n\n", nAlphaNodes, nAlphaNodes/float(m)*100.0f);

	m += nNullMoveCuts + nHashMoves + nKillerNodes[0] + nKillerNodes[1] + nCapMoves;
	fprintf(out, "TotalTreeNodes: %d\n\n\n", m);

	n = nCheckCounts-nNonCheckCounts;
	fprintf(out, "将军次数: %d\n", n);
	fprintf(out, "探测次数: %d\n", nCheckCounts);
	fprintf(out, "成功概率: %.2f%%\n\n", n/(float)nCheckCounts*100.0f);

	fprintf(out, "CheckEvasions = %d\n", nCheckEvasions);
	fprintf(out, "解将 / 将军   = %d / %d = %.2f%%\n\n", nCheckEvasions, n, nCheckEvasions/float(n)*100.0f);


	// 显示主分支
	int BoardStep[256];
	for(n=0; n<256; n++)
		BoardStep[n] = Board[n];

	static const char ChessName[14][4] = {"帥","車","炮","马","象","士","卒", "將","車","炮","馬","相","仕","兵"};

	fprintf(out, "\n主分支：PVLine***HashDepth**************************************\n");
	for(m=0; m<nPvLineNum; m++)
	{
		nSrc = (PvLine[m] & 0xFF00) >> 8;
		nDst = PvLine[m] & 0xFF;
		nCaptured = BoardStep[nDst];

		// 回合数与棋步名称
		fprintf(out, "    %2d. %s", m+1, GetStepName( PvLine[m], BoardStep ));

		// 吃子着法
		if( nCaptured )
			fprintf(out, " k-%s", ChessName[nPieceType[nCaptured]]);
		else
			fprintf(out, "     ");

		// 搜索深度
		fprintf(out, "  depth = %2d", PvLine[m]>>16);

		// 将军标志
		nCaptured = (PvLine[m] & 0xFF0000) >> 16;
		if(nCaptured)
			fprintf(out, "   Check Extended 1 ply ");
		fprintf(out, "\n");

		BoardStep[nDst] = BoardStep[nSrc];
		BoardStep[nSrc] = 0;
	}

	fprintf(out, "\n\n***********************第%2d 回合********************************\n\n", (nCurrentStep+1)/2);
	fprintf(out, "***********************引擎生成：%d 个理合着法**********************\n\n", nFirstLayerMoves);
	for(m=0; m<(unsigned int)nFirstLayerMoves; m++)
	{
		nSrc = (FirstLayerMoves[m] & 0xFF00) >> 8;
		nDst = FirstLayerMoves[m] & 0xFF;

		// 寻找主分支
		if(PvLine[0] == FirstLayerMoves[m])
		{
			fprintf(out, "*PVLINE=%d***********Nodes******History**************************\n", m+1);
			fprintf(out, "*%2d.  ", m+1);
		}
		else
			fprintf(out, "%3d.  ", m+1);

		//n = m==0 ? FirstLayerMoves[m].key : FirstLayerMoves[m].key-FirstLayerMoves[m-1].key;	// 统计分支数目
		n = FirstLayerMoves[m] >> 16;																// 统计估值
		fprintf(out, "%s = %6d    hs = %6d\n", GetStepName(FirstLayerMoves[m], Board), n, HistoryRecord[FirstLayerMoves[m]&0xFFFF]);
	}
	
	fprintf(out, "\n\n********************引擎过滤：%d个禁止着法********************************\n\n", nBanMoveNum);
	for(m=0; m<(unsigned int)nBanMoveNum; m++)
	{
		fprintf(out, "%3d. %s\n", m+1, GetStepName( BanMoveList[m], Board ));
	}

	fprintf(out, "\n\n***********************历史记录********************************\n\n", (nCurrentStep+1)/2);
	
	int MoveStr; 
	for(m=0; m<=(int)nCurrentStep; m++)
	{
		MoveStr = Coord(StepRecords[m]);
		fprintf(out, "%3d. %s  %2d  %2d  %12u\n", m, &MoveStr, (StepRecords[m] & 0xFF0000)>>16, (StepRecords[m] & 0xFF000000)>>24, nZobristBoard[m]);
	}


	// 关闭文件
	fclose(out);
}



// 循环检测：通过比较历史记录中的zobrist键值来判断。
// 改进方案：使用微型Hash表，LoopHash[zobrist & LoopMask] = zobrist  LoopMask=1023=0B1111111111  可以省去检测过程中的循环判断。
// 表现为双方的两个或多个棋子，在两个或者两个以上的位置循环往复运动。
// 根据中国象棋的棋规，先手将军出现循环，不变作负。下面列举测试程序时发现的将军循环类型：
	// 5.         10101  01010  00100
	// 6.        001010
	// 7.       1001001
	// 8.      00001010
	// 9.     100000001  101000001  010001000 000001000
	//12. 1000000000001  0000001000000
// 如此看来，各种各样的循环都可能出现。循环的第一步可以是吃子移动，以后的是非吃子移动。
// 最少5步棋可以构成循环，非吃子移动的最少数目是4。5循环是最常见的类型，6～120的循环类型，象棋棋规并没有定义。
// 循环检测，实际上起到剪枝的作用，可以减小搜索数的分支。如果不进行处理，带延伸的程序有时会无法退出。
int CSearch::RepetitionDetect(void)
{
	// 100步(50回合)无杀子，视为达到自然限着，判定为和棋
	//if(nNonCapNum >= 120)
	//	return(-120);
	
	unsigned int m, n;
	unsigned int *pBoard = &nZobristBoard[nCurrentStep-1];
	
	for(m=4; m<=nNonCapNum; m++)	
	{
		if( *pBoard == *(pBoard-m) )		// 构成循环
		{
			// 统计循环中出现的将军次数。
			CChessMove *pMove = &StepRecords[nCurrentStep-1];
			int nOwnChecks = 0;
			int nOppChecks = 0;
			for(n=0; n<=m; n++)
			{
				if((*(pMove-n)) & 0xFF000000)
				{
					if( 1 & n )
						nOppChecks ++;
					else
						nOwnChecks ++;
				}				
			}

			// 查看循环的种类
			// 我长将对手, 不一定是移动的棋子将军, 移动后其他的棋子将军也属于此类。
			if( nOwnChecks>=2 && !nOppChecks )			// 如 10101
				return 1;

			// 对手长将将我方，主动权在我方，因为最后一步我方移动。
			// 最后移动的位置，有时是被迫的，有时可以自己主动构成循环，造成对手犯规。
			else if( nOppChecks>=2 && !nOwnChecks )		// 如 01010
				return 2;
			
			// 其他情况，如长捉等，不形成将军，视为和棋。
			// 中国象棋的棋规相当复杂，例如阻挡对方某颗棋子的下一步将军，虽然循环体内不构成将军，但若不阻挡则必死无疑。
			// 根据棋规，此情况视为对方不变为负。实现此算法相当复杂。
			else
				return -int(m);
		}
	}

	return(0);
}


// 循环的返回值
// 输棋： - WINSCORE * 2	不记录于Hash表
// 赢棋： + WINSCORE * 2	不记录于Hash表
// 和棋：估值 * 藐视因子    
int CSearch::LoopValue(int Player, int ply, int nLoopStyle)
{
	// 我方循环，是由于长将对方，故而对方胜利。不记录在Hash表。
	if( nLoopStyle == 2 )
		return (ply-1)-WINSCORE*2;

	// 对方长将，我方胜利，返回胜利的分数。不记录在Hash表。
	else if( nLoopStyle == 1 )
		return WINSCORE*2-(ply-1);

	// 和棋：返回估值×藐视因子，不再继续搜索。
	else // nLoopStyle < 0 
		return int(m_Evalue.Evaluation(Player)*0.9f);

	// 藐视因子：0.9f 优势时冒进，追求赢棋；略势时保守，极力求和；势均力敌时，均衡，以免犯错。
	// 国象的作法是使用一个藐视因数delta，优势为负，弱势为正。
	// 根据国际象棋的经验，开局为0.50个兵，中局为0.25个兵，残局接近于0。
}


// 中断引擎思考
int CSearch::Interrupt(void)
{
	if(!Ponder && clock() > nMaxTimer)
		bStopThinking = 1;
	else if(!bBatch) 
	{
		switch(BusyLine(Debug)) 
		{
			case e_CommIsReady:
				fprintf(OutFile, "readyok\n");
				fflush(OutFile);
				break;

			case e_CommPonderHit:
				if(Ponder != 2) 
					Ponder = 0;
				break;

			case e_CommStop:
				bStopThinking = 1;
				break;
		}
	}

	return bStopThinking;
}