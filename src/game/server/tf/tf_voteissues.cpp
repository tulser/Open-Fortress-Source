//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base VoteController.  Handles holding and voting on issues.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "shareddefs.h"
#include "eiface.h"
#include "team.h"
#include "gameinterface.h"
#include "fmtstr.h"

#include "tf_shareddefs.h"
#include "tf_voteissues.h"
#include "tf_player.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_vote_timer_duration;

CKickIssue::CKickIssue(const char *typeString) : CBaseIssue(typeString)
{
	// NOTE: m_pzPlayerName / m_pzReason were originally just "" but that crashed the game as it was a null string, so i put in placeholder ones
	Q_snprintf(m_pzPlayerName, sizeof(m_pzPlayerName), "The reason I call it the");
	Q_snprintf(m_pzReason, sizeof(m_pzReason), "Nintendo SHITcube");
	m_iPlayerID = 0;
}

CKickIssue::~CKickIssue()
{
}

const char *CKickIssue::GetDisplayString()
{
	char result[64];
	Q_snprintf(result, sizeof(result), "#TF_vote_kick_player_%s", m_pzReason);
	char *szResult = (char*)malloc(sizeof(result));
	Q_strncpy(szResult, result, sizeof(result));
	return szResult;
}

const char *CKickIssue::GetVotePassedString()
{
	return "#TF_vote_passed_kick_player";
	//TODO: add something for "#TF_vote_passed_ban_player";
}

void CKickIssue::ListIssueDetails(CBasePlayer *a2)
{
	char s[64];
	if (true)//There should be check or something
	{
		V_snprintf(s, sizeof(s), "callvote %s <userID>\n", GetTypeString());
		ClientPrint(a2, 2, s);
	}
}

bool CKickIssue::IsEnabled()
{
	//Also some check
	return true;
}

const char * CKickIssue::GetDetailsString()
{
	int name = atoi( m_szDetailsString );

	if ( name > 0 )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( name );

		if ( pPlayer )
		{
			return pPlayer->GetPlayerName();
		}

		return "Invalid Player";
	}

	return "Unnamed";
}

void CKickIssue::OnVoteStarted()
{
	int id = atoi( m_szDetailsString );

	if ( id > 0 )
		m_iPlayerID = id;
	else
		return;

	const char *pDetails = CBaseIssue::GetDetailsString();
	const char * pch;
	pch = strrchr(pDetails, ' ');

	if ( !pch )
		return;

	int i = pch - pDetails + 1;
	Q_snprintf(m_pzReason, sizeof(m_pzReason), pDetails + i);

	char m_PlayerID[64];
	Q_snprintf(m_PlayerID, i, pDetails);

	//m_iPlayerID = atoi(m_PlayerID);
	CBasePlayer *pVoteCaller = UTIL_PlayerByIndex(m_iPlayerID);
	if (!pVoteCaller)
		return;

	Q_snprintf(m_pzPlayerName, sizeof(m_pzPlayerName), pVoteCaller->GetPlayerName());

	g_voteController->TryCastVote(pVoteCaller->entindex(), "Option2");
	CSteamID pSteamID;
	pVoteCaller->GetSteamID(&pSteamID);
	g_voteController->AddPlayerToNameLockedList(pSteamID, sv_vote_timer_duration.GetFloat(), m_iPlayerID);
}

void CKickIssue::Init()
{

}

void CKickIssue::NotifyGC(bool a2)
{
	return;
}

int CKickIssue::PrintLogData()
{
	return 0;
}

void CKickIssue::OnVoteFailed(int a2)
{
	CBaseIssue::OnVoteFailed(a2);
	PrintLogData();
}

bool CKickIssue::CreateVoteDataFromDetails(const char *s)
{
	return 0;
}

int CKickIssue::CanCallVote(int a1, char *s, int a2, int a3)
{
	return 0;
}

void CKickIssue::ExecuteCommand()
{
	if ( m_iPlayerID > 0 )
	//engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", m_hPlayerTarget->GetUserID() ) );
		engine->ServerCommand( CFmtStr( "kickid %d %s;", m_iPlayerID, "Kicked by server." ) );
	else
		DevMsg("CKickIssue has no player target for executecommand \n");
	
}

bool CKickIssue::IsTeamRestrictedVote()
{
	return 1;
}

