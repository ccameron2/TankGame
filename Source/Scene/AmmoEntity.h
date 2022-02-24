/*******************************************
	AmmoEntity.h

	Ammo entity class
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
		Ammo Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// An ammo entity inherits the ID/positioning/rendering support of the base entity class
	// and adds instance and state data. It overrides the update function to perform the Ammo
	// entity behaviour
	class CAmmoEntity : public CEntity
	{
		/////////////////////////////////////
		//	Constructors/Destructors
	public:
		// Ammo constructor intialises Ammo-specific data and passes its parameters to the base
		// class constructor
		CAmmoEntity
		(
			CEntityTemplate* entityTemplate,
			TEntityUID       UID,
			const string& name = "",
			const CVector3& position = CVector3::kOrigin,
			const CVector3& rotation = CVector3(0.0f, 0.0f, 0.0f),
			const CVector3& scale = CVector3(1.0f, 1.0f, 1.0f)
		);

		// No destructor needed


	/////////////////////////////////////
	//	Public interface
	public:

		/////////////////////////////////////
		// Update

		// Update the ammo - performs simple ammo behaviour
		// Return false if the entity is to be destroyed
		// Keep as a virtual function in case of further derivation
		virtual bool Update(TFloat32 updateTime);
	};


} // namespace gen

