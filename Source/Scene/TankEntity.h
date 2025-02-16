/*******************************************
	TankEntity.h

	Tank entity template and entity classes
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "Entity.h"
#include "CTimer.h"

namespace gen
{

/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Template Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// A tank template inherits the type, name and mesh from the base template and adds further
// tank specifications
class CTankTemplate : public CEntityTemplate
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Tank entity template constructor sets up the tank specifications - speed, acceleration and
	// turn speed and passes the other parameters to construct the base class
	CTankTemplate
	(
		const string& type, const string& name, const string& meshFilename,
		TFloat32 maxSpeed, TFloat32 acceleration, TFloat32 turnSpeed,
		TFloat32 turretTurnSpeed, TUInt32 maxHP, TUInt32 shellDamage
	) : CEntityTemplate( type, name, meshFilename )
	{
		// Set tank template values
		m_MaxSpeed = maxSpeed;
		m_Acceleration = acceleration;
		m_TurnSpeed = turnSpeed;
		m_TurretTurnSpeed = turretTurnSpeed;
		m_MaxHP = maxHP;
		m_ShellDamage = shellDamage;
	}

	// No destructor needed (base class one will do)


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	//	Getters

	TFloat32 GetMaxSpeed()
	{
		return m_MaxSpeed;
	}

	TFloat32 GetAcceleration()
	{
		return m_Acceleration;
	}

	TFloat32 GetTurnSpeed()
	{
		return m_TurnSpeed;
	}

	TFloat32 GetTurretTurnSpeed()
	{
		return m_TurretTurnSpeed;
	}

	TInt32 GetMaxHP()
	{
		return m_MaxHP;
	}

	TInt32 GetShellDamage()
	{
		return m_ShellDamage;
	}


/////////////////////////////////////
//	Private interface
private:

	// Common statistics for this tank type (template)
	TFloat32 m_MaxSpeed;        // Maximum speed for this kind of tank
	TFloat32 m_Acceleration;    // Acceleration  -"-
	TFloat32 m_TurnSpeed;       // Turn speed    -"-
	TFloat32 m_TurretTurnSpeed; // Turret turn speed    -"-

	TUInt32  m_MaxHP;           // Maximum (initial) HP for this kind of tank
	TUInt32  m_ShellDamage;     // HP damage caused by shells from this kind of tank
};



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// A tank entity inherits the ID/positioning/rendering support of the base entity class
// and adds instance and state data. It overrides the update function to perform the tank
// entity behaviour
// The shell code performs very limited behaviour to be rewritten as one of the assignment
// requirements. You may wish to alter other parts of the class to suit your game additions
// E.g extra member variables, constructor parameters, getters etc.
class CTankEntity : public CEntity
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Tank constructor intialises tank-specific data and passes its parameters to the base
	// class constructor
	CTankEntity
	(
		CTankTemplate*  tankTemplate,
		TEntityUID      UID,
		TUInt32         team,
		const string&   name = "",
		const CVector3& position = CVector3::kOrigin, 
		const CVector3& rotation = CVector3( 0.0f, 0.0f, 0.0f ),
		const CVector3& scale = CVector3( 1.0f, 1.0f, 1.0f ),
		const vector<CVector3> patrolPoints = {}
	);

	// No destructor needed
	


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	// Getters

	TFloat32 GetSpeed()
	{
		return m_Speed;
	}

	TInt32 GetHP()
	{
		return m_HP;
	}

	string GetState()
	{
		if (m_State == 0)
		{
			return "Inactive";
		}
		else if(m_State == 1)
		{
			return "Patrol";
		}
		else if (m_State == 2)
		{
			return "Aim";
		}
		else if(m_State == 3)
		{
			return "Evade";
		}
		else if (m_State == 4)
		{
			return "Empty";
		}
		else if (m_State == 5)
		{
			return "Dead";
		}
		else if (m_State == 6)
		{
			return "Guard";
		}
		else
		{
			return "Error";
		}
	}

	int GetShellCount()
	{
		return m_ShellCount;
	}

	int GetTeam()
	{
		return m_Team;
	}

	/////////////////////////////////////
	// Update

	// Update the tank - performs tank message processing and behaviour
	// Return false if the entity is to be destroyed
	// Keep as a virtual function in case of further derivation
	virtual bool Update( TFloat32 updateTime );
	 
	// Check if enemy tank is being looked at
	bool IsLookingAtEnemy(float targetAngle);

	// Find closest tank
	void FindNearestTank();

	// Find closest ammo
	void FindNearestAmmo();

	// Hit tank
	void Hit(float damage);

	// Check line of sight
	bool LineOfSight();

	//Rotate turret back to face body
	void FixTurret();
	

/////////////////////////////////////
//	Private interface
private:
	/////////////////////////////////////
	// Types

	// States available for a tank
	enum EState
	{
		Inactive,
		Patrol,
		Aim,
		Evade,
		Empty,
		Dead,
		Guard
	};
	/////////////////////////////////////
	// Data

	// The template holding common data for all tank entities
	CTankTemplate* m_TankTemplate;

	// Tank data
	TUInt32  m_Team;  // Team number for tank (to know who the enemy is)
	TFloat32 m_Speed; // Current speed (in facing direction)
	TInt32   m_HP;    // Current hit points for the tank

	// Tank state
	EState   m_State; // Current state
	TFloat32 m_Timer; // A timer used in the example update function   

	// Patrol Data
	CVector3 patrolZAmount;
	CVector3 patrolXAmount;
	CVector3 tankInitialPosition;
	CVector3 patrolPoint1;
	CVector3 patrolPoint2;
	bool reversed;
	vector<CVector3> PatrolPoints;
	int currentPatrolPoint;
	// Turret Rotation Variables
	const float pi = 3.141592653589;
	float targetAngle = pi/12; // 15 degrees
	float preciseTargetAngle = 5 * (pi / 180);
	CVector3 evadePosition;

	// Aim variables
	CTimer timer;
	bool timerStarted = false;
	bool correctAim = false;

	// Other relevant tanks
	TEntityUID nearestEnemyTank = 0;
	TFloat32 nearestTankDistance;

	// Combat variables
	int viewDistance = 100;
	int ammunition = 10;
	int m_ShellCount = 0; // Number of times tank has fired

	//Nearest ammo crate
	TEntityUID nearestAmmo = 0;
	TFloat32 nearestAmmoDistance;

	//If tank has been rotated
	bool broken = false;

	bool isGuarding = false;
	CVector3 guardPosition;
	TEntityUID tankToGuard;

};
} // namespace gen
