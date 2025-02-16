/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "ShellEntity.h"
#include "TankEntity.h"
#include "EntityManager.h"
#include "Messenger.h"

namespace gen
{

// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
extern CEntityManager EntityManager;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;

// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
// Will be needed to implement the required shell behaviour in the Update function below
extern TEntityUID GetTankUID( int team );



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Shell Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Shell constructor intialises shell-specific data and passes its parameters to the base
// class constructor
CShellEntity::CShellEntity
(
	CEntityTemplate* entityTemplate,
	TEntityUID       UID,
	const string&    name /*=""*/,
	const CVector3&  position /*= CVector3::kOrigin*/, 
	const CVector3&  rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3&  scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/,
	const int		 team,
	const int		 damage
) : CEntity( entityTemplate, UID, name, position, rotation, scale )
{
	m_Speed = 100;
	m_Team = team;
	if (m_Team == 0)
	{
		m_EnemyTeam = 1;
	}
	else
	{
		m_EnemyTeam = 0;
	}
	m_Damage = damage;
}


// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as
// one of the assignment requirements
// Return false if the entity is to be destroyed
bool CShellEntity::Update( TFloat32 updateTime )
{
	//Start timer if idle
	if (hasStarted = false)
	{
		shellTimer.Start();
		hasStarted = true;
	}

	//Destroy shell when timer runs out
	if (shellTimer.GetTime() > 2)
	{
		shellTimer.Stop();
		shellTimer.Reset();
		hasStarted = false;
		return false;
	}


	// Perform movement...
	// Move along local Z axis scaled by update time
	Matrix().MoveLocalZ(m_Speed * updateTime);


	// For each tank check if the shell is within hit distance
	EntityManager.BeginEnumEntities("", "", "Tank");
	CEntity* entity;
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* tankEntity = dynamic_cast<CTankEntity*>(entity);
		if (tankEntity->GetTeam() != m_Team && tankEntity->GetState() != "Dead")
		{
			if (Distance(Position(), tankEntity->Position()) < 2.0f)
			{
				//Damage the tank
				tankEntity->Hit(m_Damage);
				EntityManager.EndEnumEntities();
				return false;
			}
		}
	}
	EntityManager.EndEnumEntities();

	return true; // Placeholder
}


} // namespace gen
