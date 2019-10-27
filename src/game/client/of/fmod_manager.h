#ifndef FMOD_MANAGER_H
#define FMOD_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod.hpp"
using namespace FMOD;

class CFMODManager
{
public:
	CFMODManager();
	~CFMODManager();
 
	void InitFMOD();
	void ExitFMOD();

	void FadeThink();
	void Think();

	bool IsSoundPlaying( const char* pathToFileFromSoundsFolder );
	bool IsChannelPlaying( int number );

	void PlayAmbientSound( const char* pathToFileFromSoundsFolder, bool fadeIn );
	void PlayLoopingMusic(const char* pLoopingMusic,const char* pIntroMusic = NULL, bool fadeIn = false);
	void StopAmbientSound( bool fadeOut );
	void TransitionAmbientSounds( const char* pathToFileFromSoundsFolder );
	float GetSoundLenght(void);
	unsigned int GetSoundLenghtPCM( Sound *sound );

	float m_fDefaultVolume;
private:
	const char* GetFullPathToSound( const char* pathToFileFromModFolder );
	const char* GetCurrentSoundName( void );

	const char* currentSound;
	const char* introSound;
	const char* loopingSound;
	const char* newSoundFileToTransitionTo;
	bool m_bShouldTransition;
	bool m_bFadeIn;
	bool m_bFadeOut;
	bool m_bIntro;
	float m_fFadeDelay;
	float m_flSongStart;
};
 
extern CFMODManager* FMODManager();
 
#endif //FMOD_MANAGER_H