#include "AmmoEntity.h"
#include "Messenger.h"

namespace gen
{
	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;

	CAmmoEntity::CAmmoEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/,
		const int		 team
	) : CEntity(entityTemplate, UID, name, position, rotation, scale)
	{
	}

	bool CAmmoEntity::Update(TFloat32 updateTime)
	{
		// Fetch any messages
		SMessage msg;
		while (Messenger.FetchMessage(GetUID(), &msg))
		{
			// Set state variables based on received messages
			switch (msg.type)
			{
			case Msg_Collected:
				return false;
				break;
			default:
				break;
			}
		}

		//Lower crate to the ground slowly
		if (Position().y > 0)
		{
			Matrix().MoveLocalY(-0.1);
		}

		return true;
	}

}

