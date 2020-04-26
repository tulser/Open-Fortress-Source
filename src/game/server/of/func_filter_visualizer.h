//-----------------------------------------------------------------------------
// Purpose: Visualizes a respawn room to the enemy team
//-----------------------------------------------------------------------------
class CFuncFilterVisualizer : public CFuncBrush
{
	DECLARE_CLASS( CFuncFilterVisualizer, CFuncBrush );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	void	InputRoundActivate( inputdata_t &inputdata );
	int		DrawDebugTextOverlays( void );
	CFuncRespawnRoom *GetRespawnRoom( void ) { return m_hRespawnRoom; }

	virtual int		UpdateTransmitState( void );
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	void SetActive( bool bActive );

protected:
	string_t					m_iszRespawnRoomName;
	CHandle<CFuncRespawnRoom>	m_hRespawnRoom;
};

//===========================================================================================================

LINK_ENTITY_TO_CLASS( func_respawnroomvisualizer, CFuncFilterVisualizer);

BEGIN_DATADESC( CFuncFilterVisualizer )
	DEFINE_KEYFIELD( m_iszRespawnRoomName, FIELD_STRING, "respawnroomname" ),
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING,	"filtername" ),
	DEFINE_FIELD( m_hFilter,	FIELD_EHANDLE ),
	// inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFuncFilterVisualizer, DT_FuncRespawnRoomVisualizer )
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncFilterVisualizer::Spawn( void )
{
	BaseClass::Spawn();

	SetActive( true );

	SetCollisionGroup( TFCOLLISION_GROUP_RESPAWNROOMS );
}

//------------------------------------------------------------------------------
// Activate
//------------------------------------------------------------------------------
void CFuncFilterVisualizer::Activate( void ) 
{ 
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncFilterVisualizer::InputRoundActivate( inputdata_t &inputdata )
{
	if ( m_iszRespawnRoomName != NULL_STRING )
	{
		m_hRespawnRoom = dynamic_cast<CFuncRespawnRoom*>(gEntList.FindEntityByName( NULL, m_iszRespawnRoomName ));
		if ( m_hRespawnRoom )
		{
			m_hRespawnRoom->AddVisualizer( this );
			ChangeTeam( m_hRespawnRoom->GetTeamNumber() );
		}
		else
		{
			Warning("%s(%s) was unable to find func_respawnroomvisualizer named '%s'\n", GetClassname(), GetDebugName(), STRING(m_iszRespawnRoomName) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity passes the filter criteria, false if not.
// Input  : pOther - The entity to be filtered.
//-----------------------------------------------------------------------------
bool CFuncFilterVisualizer::PassesTriggerFilters(CBaseEntity *pOther)
{
	CBaseFilter *pFilter = m_hFilter.Get();
	
	return (!pFilter) ? false : pFilter->PassesFilter( this, pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CFuncFilterVisualizer::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Only transmit this entity to clients that aren't in our team
//-----------------------------------------------------------------------------
int CFuncFilterVisualizer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );

	return PassesTriggerFilters(pRecipientEntity) ? FL_EDICT_ALWAYS : FL_EDICT_DONTSEND;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncFilterVisualizer::ShouldCollideWith( int collisionGroup, int contentsMask ) const
{



	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncFilterVisualizer::SetActive( bool bActive )
{
	if ( bActive )
	{
		// We're a trigger, but we want to be solid. Out ShouldCollide() will make
		// us non-solid to members of the team that spawns here.
		RemoveSolidFlags( FSOLID_TRIGGER );
		RemoveSolidFlags( FSOLID_NOT_SOLID );	
	}
	else
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddSolidFlags( FSOLID_TRIGGER );	
	}
}
