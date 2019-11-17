#include "cbase.h"
#include "fmod_manager.h"
#include "filesystem.h"
#include "c_tf_player.h"
#include "teamplayroundbased_gamerules.h"

using namespace FMOD;

System			*pSystem;
Sound			*pSound;
SoundGroup		*pSoundGroup;
Channel			*pChannel;
ChannelGroup	*pChannelGroup;
FMOD_RESULT		result;

CFMODManager gFMODMng;
CFMODManager* FMODManager()
{
	return &gFMODMng;
}

CFMODManager::CFMODManager()
{
	m_fFadeDelay = 0.0;
	m_fDefaultVolume = 1.0f;
	m_flSongStart = 0.0f;
	newSoundFileToTransitionTo = "NULL";
	currentSound = "NULL";
	m_bShouldTransition = false;
	m_bFadeIn = false;
	m_bFadeOut = false;
}

CFMODManager::~CFMODManager()
{
	m_fFadeDelay = 0.0;
	m_fDefaultVolume = 0.0f;
	m_flSongStart = 0.0f;
	newSoundFileToTransitionTo = "NULL";
	currentSound = "NULL";
	m_bShouldTransition = false;
	m_bFadeIn = false;
	m_bFadeOut = false;
}

// Starts FMOD
void CFMODManager::InitFMOD(void)
{
	result = System_Create(&pSystem); // Create the main system object.

	if (result != FMOD_OK)
		Warning("FMOD ERROR: System creation failed!\n");
	else
		DevMsg("FMOD system successfully created.\n");

	result = pSystem->init(100, FMOD_INIT_NORMAL, 0);   // Initialize FMOD system.

	if (result != FMOD_OK)
		Warning("FMOD ERROR: Failed to initialize properly!\n");
	else
		DevMsg("FMOD initialized successfully.\n");
}

// Stops FMOD
void CFMODManager::ExitFMOD(void)
{
	result = pSystem->release();

	if (result != FMOD_OK)
		Warning("FMOD ERROR: System did not terminate properly!\n");
	else
		DevMsg("FMOD system terminated successfully.\n");
}

// Returns the full path of a specified sound file in the /sounds folder
const char* CFMODManager::GetFullPathToSound(const char* pathToFileFromModFolder)
{
	char* resultpath = new char[512];

	char fullpath[512];
	Q_snprintf(fullpath, sizeof(fullpath), "sound/%s", pathToFileFromModFolder);
	filesystem->GetLocalPath(fullpath, fullpath, sizeof(fullpath));
	Q_snprintf(resultpath, 512, fullpath);
	// convert backwards slashes to forward slashes
	for (int i = 0; i < 512; i++)
	{
		if (resultpath[i] == '\\')
			resultpath[i] = '/';
	}

	return resultpath;
}

// Returns the name of the current ambient sound being played
// If there is an error getting the name of the ambient sound or if no ambient sound is currently being played, returns "NULL"
const char* CFMODManager::GetCurrentSoundName(void)
{
	return currentSound;
}

// Handles all fade-related sound stuffs
// Called every frame when the client is in-game
void CFMODManager::FadeThink(void)
{
	if (m_bFadeOut)
	{
		if (gpGlobals->curtime >= m_fFadeDelay)
		{
			float tempvol;
			pChannel->getVolume(&tempvol);

			if (tempvol > 0.0)
			{
				pChannel->setVolume(tempvol - 0.05);
				m_fDefaultVolume = tempvol - 0.05f;
				m_fFadeDelay = gpGlobals->curtime + 0.1;
			}
			else
			{
				pChannel->setVolume(0.0);
				m_fDefaultVolume = 0.0f;
				m_bFadeOut = false;
				m_fFadeDelay = 0.0;
			}
		}
	}
	else if (m_bShouldTransition)
	{
		result = pSystem->createStream(GetFullPathToSound(newSoundFileToTransitionTo), FMOD_DEFAULT, 0, &pSound);

		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to create stream of sound '%s' ! (ERROR NUMBER: %i)\n", newSoundFileToTransitionTo, result);
			newSoundFileToTransitionTo = "NULL";
			m_bShouldTransition = false;
			return;
		}

		result = pSystem->playSound(pSound, pChannelGroup, false, &pChannel);

		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", newSoundFileToTransitionTo, result);
			newSoundFileToTransitionTo = "NULL";
			m_bShouldTransition = false;
			return;
		}

		currentSound = newSoundFileToTransitionTo;
		newSoundFileToTransitionTo = "NULL";
		m_bShouldTransition = false;
	}
	else if (m_bFadeIn)
	{
		if (gpGlobals->curtime >= m_fFadeDelay)
		{
			float tempvol;
			pChannel->getVolume(&tempvol);

			if (tempvol < 1.0)
			{
				pChannel->setVolume(tempvol + 0.05);
				m_fDefaultVolume = tempvol + 0.05f;
				m_fFadeDelay = gpGlobals->curtime + 0.1;
			}
			else
			{
				pChannel->setVolume(1.0);
				m_fDefaultVolume = 1.0f;
				m_bFadeIn = false;
				m_fFadeDelay = 0.0;
			}
		}
	}
}

static const ConVar *snd_musicvolume = NULL;
static const ConVar *snd_mute_losefocus = NULL;
// Called every frame when the client is in-game
void CFMODManager::Think(void)
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	snd_musicvolume = g_pCVar->FindVar("snd_musicvolume");
	snd_mute_losefocus = g_pCVar->FindVar("snd_mute_losefocus");

	float flVolumeMod = 1.0f;
	
	if( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		flVolumeMod -= ( gpGlobals->curtime / ( TeamplayRoundBasedRules()->m_flStartedWinState + 4.0f ) ) ;
		flVolumeMod = max( flVolumeMod, 0.0f );
	}
	
	if (pLocalPlayer && !pLocalPlayer->IsAlive())
		flVolumeMod *= 0.4f;

	if (!engine->IsActiveApp() && snd_mute_losefocus->GetBool())
		flVolumeMod = 0.0f;
	
	if( pChannelGroup )
		pChannelGroup->setVolume(m_fDefaultVolume * snd_musicvolume->GetFloat() * flVolumeMod);
}

// Compares specified ambient sound with the current ambient sound being played
// Returns true if they match, false if they do not or if no sound is being played
bool CFMODManager::IsSoundPlaying(const char* pathToFileFromSoundsFolder)
{
	const char* currentSoundPlaying = GetCurrentSoundName();

	return (strcmp(currentSoundPlaying, pathToFileFromSoundsFolder) == 0 &&
		m_flSongStart + GetSoundLenght() < gpGlobals->realtime);
}

ConVar test_sample("test_sample", "1", FCVAR_ARCHIVE);

// Compares specified ambient sound with the current ambient sound being played
// Returns true if they match, false if they do not or if no sound is being played
bool CFMODManager::IsChannelPlaying(int number)
{
	unsigned int templenght = 0;
	unsigned int position = 0;
	pSound->getLength(&templenght, FMOD_TIMEUNIT_RAWBYTES);
	pChannel->getPosition(&position, FMOD_TIMEUNIT_RAWBYTES);
	return position < (templenght - test_sample.GetInt());
}

// Compares specified ambient sound with the current ambient sound being played
// Returns true if they match, false if they do not or if no sound is being played
float CFMODManager::GetSoundLenght(void)
{
	if (!pSound)
		return 0;

	unsigned int templenght = 0;
	pSound->getLength(&templenght, FMOD_TIMEUNIT_MS);

	float lenght = 0;
	if (templenght)
		lenght = templenght * 0.001f;

	return lenght;
}

// Compares specified ambient sound with the current ambient sound being played
// Returns true if they match, false if they do not or if no sound is being played
unsigned int CFMODManager::GetSoundLenghtPCM(Sound *sound)
{
	unsigned int templenght = 0;
	sound->getLength(&templenght, FMOD_TIMEUNIT_PCM);
	return templenght - 1;
}

// Abruptly starts playing a specified ambient sound
// In most cases, we'll want to use TransitionAmbientSounds instead
void CFMODManager::PlayLoopingMusic( Channel **pNewChannel, const char* pLoopingMusic, const char* pIntroMusic, float flDelay , bool fadeIn)
{
	Channel *pTempChannel;
	
	if ( !pChannelGroup )
		pSystem->createChannelGroup("Parent", &pChannelGroup);
	
	int  outputrate = 0;
	result = pSystem->getSoftwareFormat(&outputrate, 0, 0);
	unsigned int dsp_block_len = 0;
	result = pSystem->getDSPBufferSize(&dsp_block_len, 0);
	unsigned long long clock_start = 0;
	float freq = 0;
	unsigned int slen = 0;
	
	if (pIntroMusic)
	{
		Sound *pIntroSound = NULL;
		Sound *pLoopingSound = NULL;
		
		result = pSystem->createStream(GetFullPathToSound(pIntroMusic), FMOD_CREATESTREAM, 0, &pIntroSound);
		pSystem->playSound(pIntroSound, pChannelGroup, true, &pTempChannel);

		result = pIntroSound->getDefaults(&freq, 0);
		
		result = pTempChannel->getDSPClock(0, &clock_start);

		clock_start += (dsp_block_len * 2);

		slen = (unsigned int) ( flDelay * 30000.0f );
		slen = (unsigned int)((float)slen / freq * outputrate);
		DevMsg("Initial delay lenght is %d\n", slen);
		clock_start += slen;
		
		result = pTempChannel->setDelay(clock_start, 0, false);
		result = pTempChannel->setPaused(false);

		result = pLoopingSound->setLoopPoints(0, FMOD_TIMEUNIT_PCM, GetSoundLenghtPCM(pLoopingSound), FMOD_TIMEUNIT_PCM);
		result = pSystem->createStream(GetFullPathToSound(pLoopingMusic), FMOD_LOOP_NORMAL | FMOD_CREATESTREAM , 0, &pLoopingSound);
		pSystem->playSound(pLoopingSound, pChannelGroup, true, &pTempChannel);

		result = pIntroSound->getLength(&slen, FMOD_TIMEUNIT_PCM);
		result = pIntroSound->getDefaults(&freq, 0);

		slen = (unsigned int)((float)slen / freq * outputrate);
		DevMsg("Intro delay lenght is %d\n", slen);
		clock_start += slen;

		result = pTempChannel->setDelay(clock_start, 0, false);
		result = pTempChannel->setPaused(false);

	}
	else
	{
		result = pSound->setLoopPoints(0, FMOD_TIMEUNIT_PCM, GetSoundLenghtPCM(pSound), FMOD_TIMEUNIT_PCM);
		result = pSystem->createStream(GetFullPathToSound(pLoopingMusic), FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, 0, &pSound);
		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to create stream of sound '%s' ! (ERROR NUMBER: %i)\n", GetFullPathToSound(pLoopingMusic), result);
			return;
		}
		result = pSystem->playSound(pSound, pChannelGroup, true, &pTempChannel);
		
		result = pTempChannel->getDSPClock(0, &clock_start);

		clock_start += (dsp_block_len * 2);
		pSound->getDefaults(&freq, 0);
		slen = (unsigned int) ( flDelay * 30000.0f );
		slen = (unsigned int)((float)slen / freq * outputrate);
		DevMsg("Initial delay lenght is %d\n", slen);
		clock_start += slen;

		result = pTempChannel->setDelay(clock_start, 0, false);
		
		pTempChannel->setPosition(0, FMOD_TIMEUNIT_PCM);
		pTempChannel->setPaused(false);
		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", pLoopingMusic, result);
			return;
		}
	}
	//	result = pSound->setLoopCount( -1 );
	//	

	pTempChannel->setChannelGroup(pChannelGroup);
	pTempChannel->setVolume(1.0f);
	*pNewChannel = pTempChannel;
	m_flSongStart = gpGlobals->realtime;

	if (fadeIn)
	{
		pChannel->setVolume(0.0);
		m_fDefaultVolume = 0.0f;
		m_bFadeIn = true;
	}
	else
	{
		m_fDefaultVolume = 1.0f;
	}
	currentSound = pLoopingMusic;
}

// Abruptly starts playing a specified ambient sound
// In most cases, we'll want to use TransitionAmbientSounds instead
void CFMODManager::PlayAmbientSound(const char* pathToFileFromSoundsFolder, bool fadeIn)
{
	result = pSystem->createStream(GetFullPathToSound(pathToFileFromSoundsFolder), FMOD_DEFAULT, 0, &pSound);

	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to create stream of sound '%s' ! (ERROR NUMBER: %i)\n", pathToFileFromSoundsFolder, result);
		return;
	}

	m_flSongStart = gpGlobals->realtime;
	result = pSystem->playSound(pSound, pChannelGroup, false, &pChannel);

	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", pathToFileFromSoundsFolder, result);
		return;
	}

	if (fadeIn)
	{
		pChannel->setVolume(0.0);
		m_fDefaultVolume = 0.0f;
		m_bFadeIn = true;
	}
	else
	{
		m_fDefaultVolume = 1.0f;
	}

	currentSound = pathToFileFromSoundsFolder;
}

void CFMODManager::StopAmbientSound( Channel *pNewChannel ,bool fadeOut)
{
	if (fadeOut)
	{
		pNewChannel->setVolume(1.0f);
		
		m_fDefaultVolume = 1.0f;
		m_bFadeOut = true;
	}
	else
	{
		Sound *pCurrentSound;
		pNewChannel->setVolume(0.0f);
		pNewChannel->getCurrentSound(&pCurrentSound);
		if( pCurrentSound )
		{
//			pCurrentSound->release();
		}
		pNewChannel->stop();
	}

	currentSound = "NULL";
}

void CommSndStp( void )
{
	FMODManager()->StopAllSound();
}
static ConCommand fm_stop_all_sound( "fm_stop_all_sound", CommSndStp, "Stops every channel group", FCVAR_NONE );

// Abruptly stops playing all ambient sounds
void CFMODManager::StopAllSound( void )
{
//	pChannelGroup->setVolume(0.0f);
	pChannelGroup->stop();
//	pChannelGroup->setPaused(true);
}

// Transitions between two ambient sounds if necessary
// If a sound isn't already playing when this is called, don't worry about it
void CFMODManager::TransitionAmbientSounds(const char* pathToFileFromSoundsFolder)
{
	pChannel->setVolume(1.0);
	m_fDefaultVolume = 1.0f;
	newSoundFileToTransitionTo = pathToFileFromSoundsFolder;

	m_bFadeOut = true;
	m_bShouldTransition = true;
	m_bFadeIn = true;
}