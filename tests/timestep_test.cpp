// Checks that the high frame rate conversions in CTimer give the same result per second of game time at any
// frame rate as the original code at 30 fps.
// Build and run: c++ -std=c++11 -I src/core -o timestep_test tests/timestep_test.cpp && ./timestep_test
#include <math.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t uint32;
inline float Pow(float x, float y) { return powf(x, y); }
#define FIX_BUGS
#define FIX_HIGH_FPS_BUGS

// the helpers are inline in the header, only the state needs to be set up here
#define class struct // CTimer keeps its state private
#include "Timer.h"
#undef class

uint32 CTimer::m_snTimeInMilliseconds;
uint32 CTimer::m_snTimeInMillisecondsPauseMode;
uint32 CTimer::m_snTimeInMillisecondsNonClipped;
uint32 CTimer::m_snPreviousTimeInMilliseconds;
uint32 CTimer::m_FrameCounter;
float CTimer::ms_fTimeScale;
float CTimer::ms_fTimeStep;
float CTimer::ms_fTimeStepNonClipped;
uint32 CTimer::m_LogicalFrameCounter;
uint32 CTimer::m_LogicalFramesPassed;
uint32 CTimer::m_SimFrameCounter;
uint32 CTimer::m_SimFramesPassed;
bool CTimer::m_UserPause;
bool CTimer::m_CodePause;

static int failures;

static void
Check(bool ok, const char *what, int fps, double got, double expected)
{
	if(!ok){
		printf("FAIL %s at %d fps: got %f, expected %f\n", what, fps, got, expected);
		failures++;
	}
}

// Same accumulation as CTimer::Update
static float simFrameTime;
static void
Step(float timeStep)
{
	CTimer::ms_fTimeStep = timeStep;
	CTimer::ms_fTimeStepNonClipped = timeStep;
	CTimer::m_SimFramesPassed = 0;
	simFrameTime += timeStep;
	while(simFrameTime >= CTimer::GetDefaultTimeStep()){
		simFrameTime -= CTimer::GetDefaultTimeStep();
		CTimer::m_SimFramesPassed++;
	}
	CTimer::m_SimFrameCounter += CTimer::m_SimFramesPassed;
}

static void
Reset(void)
{
	simFrameTime = 0.0f;
	CTimer::m_SimFrameCounter = 0;
	CTimer::m_SimFramesPassed = 0;
}

int
main(void)
{
	const int rates[] = { 20, 30, 60, 120, 144, 240, 360 };	// even, so half a second is a whole number of frames
	const int seconds = 10;

	// reference: the original per frame code at 30 fps
	// (the lerp is compared after half a second, it has converged by the end)
	float refMult = 1.0f, refLerp = 0.0f, refAdd = 0.0f;
	for(int i = 0; i < 30*seconds; i++){
		refMult *= 0.99f;
		if(i < 15)
			refLerp += (100.0f - refLerp) * 0.05f;
		refAdd += 2.0f;
	}

	for(int r = 0; r < (int)(sizeof(rates)/sizeof(rates[0])); r++){
		int fps = rates[r];
		int frames = fps*seconds;
		float timeStep = 50.0f / fps;
		Reset();

		float mult = 1.0f, lerp = 0.0f, add = 0.0f;
		int dueEvery8 = 0, dueEvery32 = 0;
		for(int i = 0; i < frames; i++){
			Step(timeStep);
			mult *= CTimer::ScaleFrameMultiplier(0.99f);
			if(i < fps/2)
				lerp += (100.0f - lerp) * CTimer::ScaleFrameLerp(0.05f);
			add += 2.0f * CTimer::GetTimeStepFix();
			if(CTimer::IsSimFrameDue(8)) dueEvery8++;
			if(CTimer::IsSimFrameDue(32)) dueEvery32++;
		}

		Check(fabs(mult - refMult) < 1e-3f * refMult + 1e-6f, "ScaleFrameMultiplier", fps, mult, refMult);
		Check(fabs(lerp - refLerp) < 1e-2f * refLerp, "ScaleFrameLerp", fps, lerp, refLerp);
		Check(fabs(add - refAdd) < 1e-2f * refAdd, "GetTimeStepFix", fps, add, refAdd);
		// allow one frame of rounding in the accumulated game time
		Check(CTimer::m_SimFrameCounter >= 30*seconds - 1 && CTimer::m_SimFrameCounter <= 30*seconds, "sim frame counter", fps,
		      CTimer::m_SimFrameCounter, 30*seconds);
		Check(dueEvery8 >= 30*seconds/8 - 1 && dueEvery8 <= 30*seconds/8 + 1, "IsSimFrameDue(8)", fps, dueEvery8, 30*seconds/8);
		Check(dueEvery32 >= 30*seconds/32 - 1 && dueEvery32 <= 30*seconds/32 + 1, "IsSimFrameDue(32)", fps, dueEvery32, 30*seconds/32);
		Check(CTimer::GetFrameTimeStepFix() == CTimer::GetTimeStepFix(), "GetFrameTimeStepFix", fps, CTimer::GetFrameTimeStepFix(),
		      CTimer::GetTimeStepFix());
	}

	// a lerp of 1 must stay a full step, and never overshoot
	CTimer::ms_fTimeStep = 50.0f / 144;
	Check(CTimer::ScaleFrameLerp(1.0f) == 1.0f, "ScaleFrameLerp(1)", 144, CTimer::ScaleFrameLerp(1.0f), 1.0f);
	CTimer::ms_fTimeStep = 3.0f;
	Check(CTimer::ScaleFrameLerp(0.9f) <= 1.0f, "ScaleFrameLerp overshoot", 17, CTimer::ScaleFrameLerp(0.9f), 1.0f);

	if(failures == 0)
		printf("all time step checks passed\n");
	return failures != 0;
}
