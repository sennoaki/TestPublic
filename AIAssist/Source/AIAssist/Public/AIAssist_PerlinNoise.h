// パーリンノイズクラス(FMath::PerlinNoiseだとシード値指定なく、ハッシュ固定なので)

#pragma once

#include "CoreMinimal.h"


class AIASSIST_API AIAssist_PerlinNoise
{
public:
	AIAssist_PerlinNoise();
	AIAssist_PerlinNoise(uint32 InSeed);
	~AIAssist_PerlinNoise();

	double ClacNoise(double x, double y, double z) const;

protected:
	double Fade(double t) const;
	double Lerp(double t, double a, double b) const;
	double Grad(int32 hash, double x, double y, double z) const;

	// ハッシュ関数
	int32 GetHash(int32 i) const;

private:
	TArray<int32> mHash;
};
