// パーリンノイズクラス(FMath::PerlinNoiseだとシード値指定なく、ハッシュ固定なので)

#include "AIAssist_PerlinNoise.h"
#include "Math/RandomStream.h"

AIAssist_PerlinNoise::AIAssist_PerlinNoise()
{
	mHash.Reserve(512);
	for (int32 i = 0; i < 256; ++i)
	{
		mHash.Add(i);
	}
	FRandomStream RandomStream(FMath::Rand());
	mHash.Sort([&](const int Item1, const int Item2) {
		return RandomStream.FRand() < 0.5f;
		});
	for (int32 i = 0; i < 256; ++i)
	{
		const int32 LoopValue = mHash[i];
		mHash.Add(LoopValue);
	}
}
AIAssist_PerlinNoise::AIAssist_PerlinNoise(uint32 InSeed)
{
	mHash.Reserve(512);
	for (int32 i = 0; i < 256; ++i)
	{
		mHash.Add(i);
	}
	FRandomStream RandomStream(InSeed);
	mHash.Sort([&](const int Item1, const int Item2) {
		return RandomStream.FRand() < 0.5f;
		});
	for (int32 i = 0; i < 256; ++i)
	{
		const int32 LoopValue = mHash[i];
		mHash.Add(LoopValue);
	}
}

AIAssist_PerlinNoise::~AIAssist_PerlinNoise()
{
}

double AIAssist_PerlinNoise::ClacNoise(double x, double y, double z) const
{
	// X, Y, Zを256で割っており、0から255の範囲になるようにしている
	int32 X = static_cast<int32>(FMath::Floor(x)) & 255;
	int32 Y = static_cast<int32>(FMath::Floor(y)) & 255;
	int32 Z = static_cast<int32>(FMath::Floor(z)) & 255;

	x -= FMath::Floor(x);
	y -= FMath::Floor(y);
	z -= FMath::Floor(z);

	double u = Fade(x);
	double v = Fade(y);
	double w = Fade(z);

	int32 A = mHash[X] + Y;
	int32 AA = mHash[A] + Z;
	int32 AB = mHash[A + 1] + Z;
	int32 B = mHash[X + 1] + Y;
	int32 BA = mHash[B] + Z;
	int32 BB = mHash[B + 1] + Z;

	return Lerp(w, Lerp(v, Lerp(u, Grad(mHash[AA], x, y, z),
		Grad(mHash[BA], x - 1, y, z)),
		Lerp(u, Grad(mHash[AB], x, y - 1, z),
			Grad(mHash[BB], x - 1, y - 1, z))),
		Lerp(v, Lerp(u, Grad(mHash[AA + 1], x, y, z - 1),
			Grad(mHash[BA + 1], x - 1, y, z - 1)),
			Lerp(u, Grad(mHash[AB + 1], x, y - 1, z - 1),
				Grad(mHash[BB + 1], x - 1, y - 1, z - 1))));
}

double AIAssist_PerlinNoise::Fade(double t) const
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

double AIAssist_PerlinNoise::Lerp(double t, double a, double b) const
{
	return a + t * (b - a);
}

double AIAssist_PerlinNoise::Grad(int32 hash, double x, double y, double z) const
{
	int32 h = hash & 15;
	double u = h < 8 ? x : y;
	double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

int32 AIAssist_PerlinNoise::GetHash(int32 i) const
{
	return mHash[i & 255];
}
