////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 源文件：main.cpp                                                                                 //
// *******************************************************************************************************//
// 中国象棋通用引擎----简单测试引擎示例，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： IwfWcf                                                                                        //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 控制台应用程序的入口点                                                                              //
// 2. 通过ucci协议与界面程序之间进行通讯                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "ucci.h"
#include "FenBoard.h"
#include "Search.h"

int main(int argc, char* argv[])
{
	int n;
	const char *BoolValue[2] = { "false", "true" };
	const char *ChessStyle[3] = { "solid", "normal", "risky" };
	char *BackSlashPtr;
	char BookFile[1024];
	CommEnum IdleComm;
	CommDetail Command;
	int ThisMove;
	
	printf("*******************************IwfWcf's Chess AI*********************************\n");
	printf("** 作者：IwfWcf                                                             **\n");
	printf("** 支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称UCCI) **\n");
	printf("******************************************************************************\n");
	printf("请键入ucci指令......\n");

	// 引擎接收"ucci"指令
	if(BootLine() == e_CommUcci)
	{
		// 寻找引擎所在的目录argv[0]，并且把"BOOK.DAT"默认为缺省的开局库开局库
		BackSlashPtr = strrchr(argv[0], '\\');
		if (BackSlashPtr == 0) 
			strcpy(BookFile, "BOOK.DAT");
		else
		{
			strncpy(BookFile, argv[0], BackSlashPtr + 1 - argv[0]);
			strcpy(BookFile + (BackSlashPtr + 1 - argv[0]), "BOOK.DAT");
		}

		// 调用CSearch类，构造函数初始化一些相关参数
		//a.初始化着法预产生数组
		//b.初始化Hash表，分配21+1=22级Hash表，64M
		//c.清空历史启发表
		CSearch ThisSearch;

		// 显示引擎的名称、版本、作者和使用者
		printf("\n");
		printf("id name IwfWcf's Chess AI\n");
		fflush(stdout);
		printf("id copyright 版权所有(C) 2005-2012\n");
		fflush(stdout);
		printf("id author IwfWcf\n");
		fflush(stdout);
		printf("id user 未知\n\n");
		fflush(stdout);

		// 显示引擎ucci指令的反馈信息，表示引擎所支持的选项
		// option batch %d
		printf("option batch type check default %s\n", BoolValue[ThisSearch.bBatch]);
		fflush(stdout);

		// option debug 让引擎输出详细的搜索信息，并非真正的调试模式。
		printf("option debug type check default %s\n", BoolValue[ThisSearch.Debug]);
		fflush(stdout);

		// 指定开局库文件的名称，可指定多个开局库文件，用分号“;”隔开，如不让引擎使用开局库，可以把值设成空
		ThisSearch.bUseOpeningBook = ThisSearch.m_Hash.LoadBook(BookFile);
		if(ThisSearch.bUseOpeningBook)
			printf("option bookfiles type string default %s\n", BookFile);
		else
			printf("option bookfiles type string default %s\n", 0);
		fflush(stdout);

		// 残局库名称
		printf("option egtbpaths type string default null\n");
		fflush(stdout);

		// 显示Hash表的大小
		printf("option hashsize type spin default %d MB\n", ThisSearch.m_Hash.nHashSize*2*sizeof(CHashRecord)/1024/1024);
		fflush(stdout);

		// 引擎的线程数
		printf("option threads type spin default %d\n", 0);
		fflush(stdout);

		// 引擎达到自然限着的半回合数
		printf("option drawmoves type spin default %d\n", ThisSearch.NaturalBouts);
		fflush(stdout);

		// 棋规
		printf("option repetition type spin default %d 1999年版《中国象棋竞赛规则》\n", e_RepetitionChineseRule);
		fflush(stdout);

		// 空着裁减是否打开
		printf("option pruning type check %d\n", ThisSearch);
		fflush(stdout);

		// 估值函数的使用情况
		printf("option knowledge type check %d\n", ThisSearch);
		fflush(stdout);

		// 指定选择性系数，通常有0,1,2,3四个级别。给估值函数加减一定范围内的随机数，让引擎每次走出不相同的棋。
		printf("option selectivity type spin min 0 max 3 default %d\n", ThisSearch.nSelectivity);
		fflush(stdout);

		// 指定下棋的风格，通常有solid(保守)、normal(均衡)和risky(冒进)三种
		printf("option style type combo var solid var normal var risky default %s\n", ChessStyle[ThisSearch.nStyle]);
		fflush(stdout);		

		// copyprotection 显示版权检查信息(正在检查，版权信息正确或版权信息错误)。 
		printf("copyprotection ok\n\n");
		fflush(stdout);

		// ucciok 这是ucci指令的最后一条反馈信息，表示引擎已经进入用UCCI协议通讯的状态。
		printf("ucciok\n\n");
		fflush(stdout);


		// 设定标准输出和初始局面
		ThisSearch.OutFile = stdout;	// 标准输出
		ThisSearch.fen.FenToBoard(Board, Piece, ThisSearch.Player, ThisSearch.nNonCapNum, ThisSearch.nCurrentStep, "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR r - - 0 1");
		ThisSearch.InitBitBoard(ThisSearch.Player, ThisSearch.nCurrentStep);
		printf("position fen rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR r - - 0 1\n\n");
		fflush(stdout);
		

		// 开始解释执行UCCI命令
		do 
		{
			IdleComm = IdleLine(Command, ThisSearch.Debug);
			switch (IdleComm) 
			{
				// isready 检测引擎是否处于就绪状态，其反馈信息总是readyok，该指令仅仅用来检测引擎的“指令接收缓冲区”是否能正常容纳指令。
				// readyok 表明引擎处于就绪状态(即可接收指令的状态)，不管引擎处于空闲状态还是思考状态。
				case e_CommIsReady:
					printf("readyok\n");
					fflush(stdout);
					break;

				// stop 中断引擎的思考，强制出着。后台思考没有命中时，就用该指令来中止思考，然后重新输入局面。
				case e_CommStop:
					ThisSearch.bStopThinking = 1;
					//printf("nobestmove\n");
					printf("score 0\n");
					fflush(stdout);
					break;

				// position fen 设置“内置棋盘”的局面，用fen来指定FEN格式串，moves后面跟的是随后走过的着法
				case e_CommPosition:
					// 将界面传来的Fen串转化为棋局信息
					ThisSearch.fen.FenToBoard(Board, Piece, ThisSearch.Player, ThisSearch.nNonCapNum, ThisSearch.nCurrentStep, Command.Position.FenStr);
					ThisSearch.InitBitBoard(ThisSearch.Player, ThisSearch.nCurrentStep);

					// 将局面走到当前，主要是为了更新着法记录，用于循环检测。
					for(n=0; n<Command.Position.MoveNum; n++)
					{
						ThisMove = Move(Command.Position.CoordList[n]);
						if( !ThisMove )
							break;

						ThisSearch.MovePiece( ThisMove );
						ThisSearch.StepRecords[ThisSearch.nCurrentStep-1] |= ThisSearch.Checking(ThisSearch.Player) << 24;
					}

					ThisSearch.nBanMoveNum = 0;
					break;

				// banmoves 为当前局面设置禁手，以解决引擎无法处理的长打问题。当出现长打局面时，棋手可以操控界面向引擎发出禁手指令。
				case e_CommBanMoves:
					ThisSearch.nBanMoveNum = Command.BanMoves.MoveNum;
					for(n=0; n<Command.BanMoves.MoveNum; n++)
						ThisSearch.BanMoveList[n] = Move(Command.BanMoves.CoordList[n]);
					break;

				// setoption 设置引擎各种参数
				case e_CommSetOption:
					switch(Command.Option.Type) 
					{
						// setoption batch %d
						case e_OptionBatch:
							ThisSearch.bBatch = (Command.Option.Value.Check == e_CheckTrue);
							printf("option batch type check default %s\n", BoolValue[ThisSearch.bBatch]);
							fflush(stdout);
							break;

						// setoption debug %d 让引擎输出详细的搜索信息，并非真正的调试模式。
						case e_OptionDebug:
							ThisSearch.Debug = (Command.Option.Value.Check == e_CheckTrue);
							printf("option debug type check default %s\n", BoolValue[ThisSearch.Debug]);
							fflush(stdout);
							break;

						// setoption bookfiles %s  指定开局库文件的名称，可指定多个开局库文件，用分号“;”隔开，如不让引擎使用开局库，可以把值设成空
						case e_OptionBookFiles:
							strcpy(BookFile, Command.Option.Value.String);
							printf("option bookfiles type string default %s\n", BookFile);
							fflush(stdout);
							break;

						// setoption egtbpaths %s  指定残局库文件的名称，可指定多个残局库路径，用分号“;”隔开，如不让引擎使用残局库，可以把值设成空
						case e_OptionEgtbPaths:
							// 引擎目前不支持开局库
							printf("option egtbpaths type string default null\n");
							fflush(stdout);
							break;

						// setoption hashsize %d  以MB为单位规定Hash表的大小，-1表示让引擎自动分配Hash表。1～1024MB
						// 象堡界面有个Bug，每次设置引擎时，这个命令应在开局库的前面
						case e_OptionHashSize:
							// -1MB(自动), 0MB(自动), 1MB(16), 2MB(17), 4MB(18), 8MB(19), 16MB(20), 32MB(21), 64MB(22), 128MB(23), 256MB(24), 512MB(25), 1024MB(26)
							if( Command.Option.Value.Spin <= 0)
								n = 22;		// 缺省情况下，引擎自动分配(1<<22)*16=64MB，红与黑两各表，双方各一半。
							else
							{
								n = 15;											// 0.5 MB = 512 KB 以此为基数
								while( Command.Option.Value.Spin > 0 )
								{
									Command.Option.Value.Spin >>= 1;			// 每次除以2，直到为0
									n ++;
								}
							}								

							// 应加入内存检测机制，引擎自动分配时，Hash表大小为可用内存的1/2。
							ThisSearch.m_Hash.DeleteHashTable();					// 必须使用delete先清除旧的Hash表
							ThisSearch.m_Hash.NewHashTable(n > 26 ? 26 : n, 12);	// 为引擎分配新的Hash表
							printf("option hashsize type spin default %d MB\n", ThisSearch.m_Hash.nHashSize*2*sizeof(CHashRecord)/1024/1024);	// 显示实际分配的Hash表大小，单位：MB
							fflush(stdout);

							ThisSearch.m_Hash.ClearHashTable();
							ThisSearch.bUseOpeningBook = ThisSearch.m_Hash.LoadBook(BookFile);
							break;

						// setoption threads %d	      引擎的线程数，为多处理器并行运算服务
						case e_OptionThreads:
							// ThisSearch.nThreads = Command.Option.Value.Spin;		// 0(auto),1,2,4,8,16,32
							printf("option drawmoves type spin default %d\n", 0);
							fflush(stdout);
							break;

						// setoption drawmoves %d	  达到自然限着的回合数:50,60,70,80,90,100，象堡已经自动转化为半回合数
						case e_OptionDrawMoves:
							ThisSearch.NaturalBouts = Command.Option.Value.Spin;
							printf("option drawmoves type spin default %d\n", ThisSearch.NaturalBouts);
							fflush(stdout);
							break;

						// setoption repetition %d	  处理循环的棋规，目前只支持“中国象棋棋规1999”
						case e_OptionRepetition:
							// ThisSearch.nRepetitionStyle = Command.Option.Value.Repetition;
							// e_RepetitionAlwaysDraw  不变作和
							// e_RepetitionCheckBan    禁止长将
							// e_RepetitionAsianRule   亚洲规则
							// e_RepetitionChineseRule 中国规则（缺省）
							printf("option repetition type spin default %d", e_RepetitionChineseRule);
							printf("  引擎目前支持1999年版《中国象棋竞赛规则》\n");
							fflush(stdout);
							break;

						// setoption pruning %d，“空着向前裁剪”是否打开
						case e_OptionPruning:
							ThisSearch.bPruning = Command.Option.Value.Scale;
							printf("option pruning type check %d\n", ThisSearch);
							fflush(stdout);
							break;

						// setoption knowledge %d，估值函数的使用
						case e_OptionKnowledge:
							ThisSearch.bKnowledge = Command.Option.Value.Scale;
							printf("option knowledge type check %d\n", ThisSearch);
							fflush(stdout);
							break;

						// setoption selectivity %d  指定选择性系数，通常有0,1,2,3四个级别
						case e_OptionSelectivity:
							switch (Command.Option.Value.Scale)
							{
								case e_ScaleNone:
									ThisSearch.SelectMask = 0;
									break;
								case e_ScaleSmall:
									ThisSearch.SelectMask = 1;
									break;
								case e_ScaleMedium:
									ThisSearch.SelectMask = 3;
									break;
								case e_ScaleLarge:
									ThisSearch.SelectMask = 7;
									break;
								default:
									ThisSearch.SelectMask = 0;
									break;
							}
							printf("option selectivity type spin min 0 max 3 default %d\n", ThisSearch.SelectMask);
							fflush(stdout);
							break;

						// setoption style %d  指定下棋的风格，通常有solid(保守)、normal(均衡)和risky(冒进)三种
						case e_OptionStyle:
							ThisSearch.nStyle = Command.Option.Value.Style;
							printf("option style type combo var solid var normal var risky default %s\n", ChessStyle[Command.Option.Value.Style]);
							fflush(stdout);
							break;						

						// setoption loadbook  UCCI界面ElephantBoard在每次新建棋局时都会发送这条指令
						case e_OptionLoadBook:
							ThisSearch.m_Hash.ClearHashTable();
							ThisSearch.bUseOpeningBook = ThisSearch.m_Hash.LoadBook(BookFile);
							
							if(ThisSearch.bUseOpeningBook)
								printf("option loadbook succeed. %s\n", BookFile);		// 成功
							else
								printf("option loadbook failed! %s\n", "Not found file BOOK.DAT");				// 没有开局库
							fflush(stdout);
							printf("\n\n");
							fflush(stdout);
							break;

						default:
							break;
					}
					break;

				// Prepare timer strategy according to "go depth %d" or "go ponder depth %d" command
				case e_CommGo:
				case e_CommGoPonder:
					switch (Command.Search.Mode)
					{
						// 固定深度
						case e_TimeDepth:
							ThisSearch.Ponder = 2;
							ThisSearch.MainSearch(Command.Search.DepthTime.Depth);
							break;

						// 时段制： 分配时间 = 剩余时间 / 要走的步数
						case e_TimeMove:							
							ThisSearch.Ponder = (IdleComm == e_CommGoPonder ? 1 : 0);
							printf("%d\n", Command.Search.TimeMode.MovesToGo);
							ThisSearch.MainSearch(127, Command.Search.DepthTime.Time * 1000 / Command.Search.TimeMode.MovesToGo, Command.Search.DepthTime.Time * 1000);
							break;

						// 加时制： 分配时间 = 每步增加的时间 + 剩余时间 / 20 (即假设棋局会在20步内结束)
						case e_TimeInc:
							ThisSearch.Ponder = (IdleComm == e_CommGoPonder ? 1 : 0);
							ThisSearch.MainSearch(127, (Command.Search.DepthTime.Time + Command.Search.TimeMode.Increment * 20) * 1000 / 20, Command.Search.DepthTime.Time * 1000);
							break;

						default:
							break;
					}
					break;
			}
		} while (IdleComm != e_CommQuit);

		printf("bye\n");
		fflush(stdout);
	}

	return 0;
}

