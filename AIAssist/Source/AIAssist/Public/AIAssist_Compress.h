// ÉfÅ[É^à≥èkån

#pragma once

#include "CoreMinimal.h"

//8bit(0Å`200)Ç≈0.0Å`1.0Çï\åª
struct FAIAssist_CompressOneFloat
{
	uint8 mData = 0;

	float	GetValue() const
	{
		return static_cast<float>(mData) * 0.005f;
	}
	uint8	GetData() const { return mData; }
	void	SetData(const uint8 InData){mData = InData;}
	void	SetData(const float InValue)
	{
		mData = static_cast<uint8>(FMath::Clamp(static_cast<int32>(InValue * 200.f),0,200));
	}
};

//32bitÇ≈0.0Å`1.0ÇÃx,y,zÇÃVectorÇï\åª
union FAIAssist_CompressOneVector
{
	int32 mData = 0;
	struct FCompressVector
	{//0.0Å`1.0Ç0Å`1000Ç≈ï\åª
		int32 mX : 10;
		int32 mY : 10;
		int32 mZ : 10;
		int32 :2;

		FVector GetVector() const
		{
			return FVector(
				static_cast<float>(mX) * 0.001f,
				static_cast<float>(mY) * 0.001f,
				static_cast<float>(mZ) * 0.001f
			);
		}
		void SetVector(const FVector& InVector)
		{
			mX = FMath::Clamp(static_cast<int32>(InVector.X * 1000.f), 0, 1000);
			mY = FMath::Clamp(static_cast<int32>(InVector.Y * 1000.f), 0, 1000);
			mZ = FMath::Clamp(static_cast<int32>(InVector.Z * 1000.f), 0, 1000);
		}
	};
	FCompressVector	mCompressVector;

	FVector	GetVector() const
	{
		return mCompressVector.GetVector();
	}
	int32	GetData() const{return mData;}
	void	SetData(int32 InData)
	{
		mData = InData;
	}
	void	SetData(const FVector& InVector)
	{
		mCompressVector.SetVector(InVector);
	}
};

//int32ÇÃTArrayÇbitîzóÒë„ÇÌÇËÇ…Ç∑ÇÈ
struct FAIAssist_CompressBitArray
{
	static bool	GetBool(const TArray<int32>& InArray, const int32 InIndex)
	{
		const int32 ArrayIndex = InIndex/32;
		if (ArrayIndex >= InArray.Num())
		{
			return false;
		}
		const int32 TargetInt = InArray[ArrayIndex];
		const int32 IndexShift = InIndex - ArrayIndex*32;
		return !!(TargetInt & 1<<IndexShift);
	}
	static void	SetBool(TArray<int32>& OutArray, const int32 InIndex, const bool bInBool)
	{
		const int32 ArrayIndex = InIndex / 32;
		while (ArrayIndex >= OutArray.Num())
		{
			OutArray.Add(0);
		}
		int32& TargetInt = OutArray[ArrayIndex];
		const int32 IndexShift = InIndex - ArrayIndex * 32;
		if(bInBool)
		{
			TargetInt = TargetInt | 1 << IndexShift;
		}
		else
		{
			TargetInt = TargetInt & ~(1 << IndexShift);
		}
	}
};