/*******************************************
	TankEntity.cpp

	Tank entity template and entity classes
********************************************/

// Additional technical notes for the assignment:
// - Each tank has a team number (0 or 1), HP and other instance data - see the end of TankEntity.h
//   You will need to add other instance data suitable for the assignment requirements
// - A function GetTankUID is defined in TankAssignment.cpp and made available here, which returns
//   the UID of the tank on a given team. This can be used to get the enemy tank UID
// - Tanks have three parts: the root, the body and the turret. Each part has its own matrix, which
//   can be accessed with the Matrix function - root: Matrix(), body: Matrix(1), turret: Matrix(2)
//   However, the body and turret matrix are relative to the root's matrix - so to get the actual 
//   world matrix of the body, for example, we must multiply: Matrix(1) * Matrix()
// - Vector facing work similar to the car tag lab will be needed for the turret->enemy facing 
//   requirements for the Patrol and Aim states
// - The CMatrix4x4 function DecomposeAffineEuler allows you to extract the x,y & z rotations
//   of a matrix. This can be used on the *relative* turret matrix to help in rotating it to face
//   forwards in Evade state
// - The CShellEntity class is simply an outline. To support shell firing, you will need to add
//   member data to it and rewrite its constructor & update function. You will also need to update 
//   the CreateShell function in EntityManager.cpp to pass any additional constructor data required
// - Destroy an entity by returning false from its Update function - the entity manager wil perform
//   the destruction. Don't try to call DestroyEntity from within the Update function.
// - As entities can be destroyed, you must check that entity UIDs refer to existant entities, before
//   using their entity pointers. The return value from EntityManager.GetEntity will be NULL if the
//   entity no longer exists. Use this to avoid trying to target a tank that no longer exists etc.

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
// Will be needed to implement the required tank behaviour in the Update function below
extern TEntityUID GetTankUID( int team );



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Tank constructor intialises tank-specific data and passes its parameters to the base
// class constructor
CTankEntity::CTankEntity
(
	CTankTemplate*  tankTemplate,
	TEntityUID      UID,
	TUInt32         team,
	const string&   name /*=""*/,
	const CVector3& position /*= CVector3::kOrigin*/, 
	const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity( tankTemplate, UID, name, position, rotation, scale )
{
	m_TankTemplate = tankTemplate;

	// Tanks are on teams so they know who the enemy is
	m_Team = team;

	// Initialise other tank data and state
	m_Speed = 0.0f;
	m_HP = m_TankTemplate->GetMaxHP();
	m_State = Inactive;
	m_Timer = 0.0f;

	//Initialise Patrol Data
	patrolAmount = { 0,0,30 };
	tankInitialPosition = Position();
	patrolPoint1 = tankInitialPosition + patrolAmount;
	patrolPoint2 = tankInitialPosition - patrolAmount;
	reversed = false;
}


// Update the tank - controls its behaviour. The shell code just performs some test behaviour, it
// is to be rewritten as one of the assignment requirements
// Return false if the entity is to be destroyed
bool CTankEntity::Update( TFloat32 updateTime )
{
	// Fetch any messages
	SMessage msg;
	while (Messenger.FetchMessage( GetUID(), &msg ))
	{
		
		// Set state variables based on received messages
		switch (msg.type)
		{
			case Msg_Stop:
				m_State = Inactive;
				break;
			case Msg_Start:
				m_State = Patrol;
				break;
			case Msg_Hit:
				m_HP -= 20;
				EntityManager.BeginEnumEntities("", "", "Tank");
				CEntity* entity;
				while (entity = EntityManager.EnumEntity())
				{
					CTankEntity* tankEntity = dynamic_cast<CTankEntity*>(entity);
					SMessage msg;
					msg.from = GetUID();
					msg.type = Msg_Help;
					Messenger.SendMessageA(entity->GetUID(), msg);
				}
				break;
			case Msg_Evade:
				evadePosition = Position() + CVector3{ float(Random(1,40)), 0, float(Random(1,40)) };
				m_State = Evade;
				break;
			case Msg_Help:
				tankToHelp = msg.from;
				if (EntityManager.GetEntity(tankToHelp))
				{
					if (Distance(Position(), EntityManager.GetEntity(tankToHelp)->Position()) < viewDistance)
					{
						m_State = Aim;
					}
				}				
				break;
		}
	}

	// Tank behaviour
	if (m_State == Patrol)
	{
		m_Speed = 10;
		if (Distance(Position(), patrolPoint1) < 2.0f)
		{
			reversed = true;
		}
		if (Distance(Position(), patrolPoint2) < 2.0f)
		{
			reversed = false;
		}
		if (!reversed)
		{
			Matrix(0).FaceTarget(patrolPoint1);
		}
		else
		{
			Matrix(0).FaceTarget(patrolPoint2);

		}
		Matrix(2).RotateY(0.01);

		if (IsLookingAtEnemy(ToRadians(15)))
		{
			m_State = Aim;
		}
	}
	else if (m_State == Aim)
	{
		//Start timer if idle
		if (timerStarted == false)
		{
			timer.Start();
			timerStarted = true;
		}

		m_Speed = 0;

		if (timer.GetTime() < 1)
		{			
			if (!IsLookingAtEnemy(ToRadians(2)))
			{			
				Matrix(2).RotateY(0.015);
			}	
		}
		else
		{
			timer.Stop();
			timerStarted = false;
			timer.Reset();

			//Fire a shell and change to evade state
			CVector3 turretRotation;
			(Matrix(0) * Matrix(2)).DecomposeAffineEuler(NULL, &turretRotation, NULL);
			if (ammunition > 0)
			{
				EntityManager.CreateShell("Shell Type 1", "", Position(), turretRotation, { 1,1,1 }, m_Team);
				m_ShellCount++;
				ammunition--;
				SMessage msg;
				msg.type = Msg_Evade;
				Messenger.SendMessageA(GetUID(), msg);
			}
			else
			{
				m_State = Empty;
			}
				
		}
	}
	else if (m_State == Evade)
	{
		m_Speed = 10;
		Matrix(0).FaceTarget(evadePosition);
		CVector3 bodyRotation;
		Matrix(0).DecomposeAffineEuler(NULL, &bodyRotation, NULL);
		CVector3 turretRotation;
		(Matrix(0)*Matrix(2)).DecomposeAffineEuler(NULL, &turretRotation, NULL);
		if (turretRotation.y < bodyRotation.y - ToRadians(3))
		{			
			Matrix(2).RotateY(0.02);
		}
		else if (turretRotation.y > bodyRotation.y + ToRadians(3))
		{
			Matrix(2).RotateY(-0.02);
		}
		if (Distance(Position(), evadePosition) < 2.0f)
		{
			m_State = Patrol;
		}

	}
	else if (m_State = Empty)
	{
		m_Speed = 0;
		FindNearestAmmo();
		if (nearestAmmo != 0)
		{
			if (EntityManager.GetEntity(nearestAmmo))
			{
				auto nearestAmmoPosition = EntityManager.GetEntity(nearestAmmo)->Position();
				if (nearestAmmoPosition.y < 1)
				{
					Matrix(0).FaceTarget(nearestAmmoPosition);
					m_Speed = 10;
				}
				if (Distance(Position(), nearestAmmoPosition) < 2)
				{
					ammunition += 10;
					m_State = Patrol;
					SMessage msg;
					msg.from = GetUID();
					msg.type = Msg_Collected;
					Messenger.SendMessageA(nearestAmmo, msg);
				}

			}				
		}		
	}
	else
	{
		m_Speed = 0;
	}


	if (m_HP <= 0)
	{
		return false;
	}

	// Perform movement...
	// Move along local Z axis scaled by update time
	Matrix().MoveLocalZ( m_Speed * updateTime );

	return true; // Don't destroy the entity
}

bool CTankEntity::IsLookingAtEnemy(float targetAngle)
{
	FindNearestTank();

	CMatrix4x4 turretWorldMatrix = Matrix(2) * Matrix();

	if (EntityManager.GetEntity(nearestEnemyTank) != 0)
	{
		CVector3 targetPosition = EntityManager.GetEntity(nearestEnemyTank)->Matrix().Position();
		auto distanceVector = targetPosition - Position();
		if (Distance(targetPosition, Position()) > viewDistance)
		{
			return false;
		}
		distanceVector.Normalise();
		auto turretFacingVector = turretWorldMatrix.ZAxis();
		turretFacingVector.Normalise();
		auto dot = distanceVector.Dot(turretFacingVector);
		if (acos(dot) <= targetAngle)
		{
			return true;
		}
	}

	return false;

}

void CTankEntity::FindNearestTank()
{
	EntityManager.BeginEnumEntities("", "", "Tank");
	CEntity* entity;
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* tankEntity = dynamic_cast<CTankEntity*>(entity);
		if (tankEntity->GetTeam() != m_Team)
		{
			if (EntityManager.GetEntity(nearestEnemyTank) == 0)
			{
				nearestEnemyTank = 0;
			}
			if (nearestEnemyTank == 0)
			{
				nearestTankDistance = 9999999;
			}
			else
			{
				nearestTankDistance = Distance(Position(), EntityManager.GetEntity(nearestEnemyTank)->Position());
			}
			auto tankDistance = Distance(Position(), tankEntity->Position());
			if (Distance(Position(), tankEntity->Position()) < viewDistance)
			{
				if (tankDistance < nearestTankDistance)
				{
					nearestEnemyTank = entity->GetUID();
				}
			}
		}
	}
	EntityManager.EndEnumEntities();
}

void CTankEntity::FindNearestAmmo()
{
	EntityManager.BeginEnumEntities("", "", "Ammo");
	CEntity* entity;
	while (entity = EntityManager.EnumEntity())
	{
		if (EntityManager.GetEntity(nearestAmmo) == 0)
		{
			nearestAmmo = 0;
		}
		if (nearestAmmo == 0)
		{
			nearestAmmoDistance = 9999999;
		}
		else
		{
			nearestAmmoDistance = Distance(Position(), EntityManager.GetEntity(nearestAmmo)->Position());
		}
		auto tankDistance = Distance(Position(), entity->Position());
		if (tankDistance < nearestAmmoDistance)
		{
			nearestAmmo = entity->GetUID();
		}		
	}
	EntityManager.EndEnumEntities();
}



} // namespace gen
