////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 头文件：Evaluation.h                                                                                   //
// *******************************************************************************************************//
// 中国象棋通用引擎----简单测试引擎示例，支持《中国象棋通用引擎协议》(Universal Chinese Chess Protocol，简称ucci) //
// 作者： wying                                                                                        //
// *******************************************************************************************************//
// 功能：                                                                                                 //
// 1. 先手分=4                                                                                            //
// 2. 棋子价值                                                                                            //
// 3. 位置分                                                                                              //
// 4.                                                                                                     //
// 5.                                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

extern const int nOffensiveValue;
extern const int BasicValues[14];
extern int Evalue[2];							// 双方估值：0=黑方, 1=红方	
extern int PositionValue[14][256];				// 局面估值


class CEvaluation
{
public:
	CEvaluation(void);
	~CEvaluation(void);

public:
	int Evaluation(int Player, int alpha=-32767, int beta=32767);
};

// MSB = Most Significant Bit
// It is a smart algorithm equivalent to BSR instruction in 80386 processor
//inline int Msb32(unsigned long Arg) 
//{
//	int RetVal = 0;
//	if (Arg & 0xffff0000) 
//	{
//		RetVal += 16;
//		Arg &= 0xffff0000;
//	}
//	if (Arg & 0xff00ff00) 
//	{
//		RetVal += 8;
//		Arg &= 0xff00ff00;
//	}
//	if (Arg & 0xf0f0f0f0) 
//	{
//		RetVal += 4;
//		Arg &= 0xf0f0f0f0;
//	}
//	if (Arg & 0xcccccccc) 
//	{
//		RetVal += 2;
//		Arg &= 0xcccccccc;
//	}
//	if (Arg & 0xaaaaaaaa) 
//	{
//		RetVal += 1;
//	}
//
//	return RetVal;
//}
//
//
//// LSB = Least Significant Bit
//// It is a smart algorithm equivalent to BSF instruction in 80386 processor
//
//inline int Lsb32(unsigned long Arg) 
//{
//	int RetVal = 31;
//	if (Arg & 0x0000ffff) 
//	{
//		RetVal -= 16;
//		Arg &= 0x0000ffff;
//	}
//	if (Arg & 0x00ff00ff) 
//	{
//		RetVal -= 8;
//		Arg &= 0x00ff00ff;
//	}
//	if (Arg & 0x0f0f0f0f) 
//	{
//		RetVal -= 4;
//		Arg &= 0x0f0f0f0f;
//	}
//	if (Arg & 0x33333333) 
//	{
//		RetVal -= 2;
//		Arg &= 0x33333333;
//	}
//	if (Arg & 0x55555555) 
//	{
//		RetVal -= 1;
//	}
//
//	return RetVal;
//}
//
// A smart algorithm to count set (1) bits
inline int Count32(unsigned long Arg)
{
	int t;

	t = ((Arg >> 1) & 0x55555555) + (Arg & 0x55555555);
	t = ((t >> 2) & 0x33333333) + (t & 0x33333333);
	t = ((t >> 4) & 0x0f0f0f0f) + (t & 0x0f0f0f0f);
	t = ((t >> 8) & 0x00ff00ff) + (t & 0x00ff00ff);
	return (t >> 16) + (t & 0x0000ffff);
}