#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "Psapi.h"
#pragma comment(lib, "psapi.lib")
#endif

#include "cbase.h"
#include "of_modelloader.h"

IModelLoader* modelloader = NULL;

#define MODELLOADER_ADDRESS 0x005AB728

void SetupModelLoader()
{
#ifdef _WIN32
    HMODULE handle = GetModuleHandleA( "engine.dll" );
    if ( !handle )
        return;

    MODULEINFO info;
    GetModuleInformation( GetCurrentProcess(), handle, &info, sizeof( info ) );

    modelloader = ( IModelLoader* )( ( uintptr_t )info.lpBaseOfDll + ( uintptr_t )MODELLOADER_ADDRESS );
#endif
}

void CheckAndPreserveModel( model_t* mod )
{
#ifdef _WIN32
    if ( modelloader->IsLoaded( mod ) )
    {
#ifdef GAME_DLL
        modelloader->GetModelForName( modelloader->GetName( mod ), IModelLoader::FMODELLOADER_SERVER );
#else
        modelloader->GetModelForName( modelloader->GetName( mod ), IModelLoader::FMODELLOADER_CLIENT );
#endif
    }
#endif
}

#ifdef _WIN32
CON_COMMAND_F( modelloader_print, "", FCVAR_CHEAT )
{
    modelloader->Print();
}
#endif