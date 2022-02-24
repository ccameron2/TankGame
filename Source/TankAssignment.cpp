/*******************************************
	TankAssignment.cpp

	Shell scene and game functions
********************************************/

#include <sstream>
#include <string>
using namespace std;

#include <d3d10.h>
#include <d3dx10.h>

#include "Defines.h"
#include "CVector3.h"
#include "Camera.h"
#include "Light.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "XML/CParseLevel.h"
#include "TankAssignment.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Control speed
const float CameraRotSpeed = 2.0f;
float CameraMoveSpeed = 80.0f;

// Amount of time to pass before calculating new average update time
const float UpdateTimePeriod = 1.0f;


//-----------------------------------------------------------------------------
// Global system variables
//-----------------------------------------------------------------------------

// Get reference to global DirectX variables from another source file
extern ID3D10Device*           g_pd3dDevice;
extern IDXGISwapChain*         SwapChain;
extern ID3D10DepthStencilView* DepthStencilView;
extern ID3D10RenderTargetView* BackBufferRenderTarget;
extern ID3DX10Font*            OSDFont;

// Actual viewport dimensions (fullscreen or windowed)
extern TUInt32 ViewportWidth;
extern TUInt32 ViewportHeight;

// Current mouse position
extern TUInt32 MouseX;
extern TUInt32 MouseY;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;


//-----------------------------------------------------------------------------
// Global game/scene variables
//-----------------------------------------------------------------------------

// Entity manager
CEntityManager EntityManager;
CParseLevel LevelParser(&EntityManager);

// Other scene elements
const int NumLights = 2;
CLight*  Lights[NumLights];
SColourRGBA AmbientLight;
CCamera* MainCamera;
int treeNum = 100;

// Sum of recent update times and number of times in the sum - used to calculate
// average over a given time period
float SumUpdateTimes = 0.0f;
int NumUpdateTimes = 0;
float AverageUpdateTime = -1.0f; // Invalid value at first

//Picking
TEntityUID NearestTankEntity;
float NearestEntityDistance;
bool ToggleExtendedInfo = true;
bool GrabbedTank = false;
int PickDist = 50;

//Chase camera
bool ChaseCamera = false;
TEntityUID ChasedTank;

//Ammo variables
CTimer AmmoTimer;
bool AmmoTimerStarted = true;
int AmmoTimerDuration = 0;


//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	//////////////////////////////////////////////
	// Prepare render methods
	InitInput();
	InitialiseMethods();


	//Load entities from xml
	LevelParser.ParseFile("Entities.xml");

	//Create tree entities
	for (int i = 0; i < treeNum; i++)
	{
		EntityManager.CreateEntity("Tree", "Tree " + to_string(i));
	}

	//For each tree set position and rotation randomly
	EntityManager.BeginEnumEntities("", "Tree", "Scenery");
	CEntity* entity = 0;
	while (entity = EntityManager.EnumEntity())
	{
		entity->Position() = CVector3(Random(-200.0f, 30.0f), 0.0f, Random(40.0f, 150.0f));
		entity->Matrix().RotateY(Random(0.0f, 2.0f * kfPi));
	}

	/////////////////////////////
	// Camera / light setup

	// Set camera position and clip planes
	MainCamera = new CCamera(CVector3(0.0f, 30.0f, -100.0f), CVector3(ToRadians(15.0f), 0, 0));
	MainCamera->SetNearFarClip(1.0f, 20000.0f);

	// Sunlight and light in building
	Lights[0] = new CLight(CVector3(-5000.0f, 4000.0f, -10000.0f), SColourRGBA(1.0f, 0.9f, 0.6f), 15000.0f);
	Lights[1] = new CLight(CVector3(6.0f, 7.5f, 40.0f), SColourRGBA(1.0f, 0.0f, 0.0f), 1.0f);

	// Ambient light level
	AmbientLight = SColourRGBA(0.6f, 0.6f, 0.6f, 1.0f);

	return true;
}


// Release everything in the scene
void SceneShutdown()
{
	// Release render methods
	ReleaseMethods();

	// Release lights
	for (int light = NumLights - 1; light >= 0; --light)
	{
		delete Lights[light];
	}

	// Release camera
	delete MainCamera;

	// Destroy all entities
	EntityManager.DestroyAllEntities();
	EntityManager.DestroyAllTemplates();
}


//-----------------------------------------------------------------------------
// Game Helper functions
//-----------------------------------------------------------------------------

// Get UID of tank A (team 0) or B (team 1)
//TEntityUID GetTankUID(int team)
//{
//	return (team == 0) ? TankA : TankB;
//}


//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Draw one frame of the scene
void RenderScene( float updateTime )
{
	// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width  = ViewportWidth;
	vp.Height = ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	// Select the back buffer and depth buffer to use for rendering
	g_pd3dDevice->OMSetRenderTargets( 1, &BackBufferRenderTarget, DepthStencilView );
	
	// Clear previous frame from back buffer and depth buffer
	g_pd3dDevice->ClearRenderTargetView( BackBufferRenderTarget, &AmbientLight.r );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// Update camera aspect ratio based on viewport size - for better results when changing window size
	MainCamera->SetAspect( static_cast<TFloat32>(ViewportWidth) / ViewportHeight );

	// Set camera and light data in shaders
	MainCamera->CalculateMatrices();
	SetCamera(MainCamera);
	SetAmbientLight(AmbientLight);
	SetLights(&Lights[0]);

	// Render entities and draw on-screen text
	EntityManager.RenderAllEntities();
	RenderSceneText( updateTime );

    // Present the backbuffer contents to the display
	SwapChain->Present( 0, 0 );
}


// Render a single text string at the given position in the given colour, may optionally centre it
void RenderText( const string& text, int X, int Y, float r, float g, float b, bool centre = false )
{
	RECT rect;
	if (!centre)
	{
		SetRect( &rect, X, Y, 0, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
	else
	{
		SetRect( &rect, X - 100, Y, X + 100, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_CENTER | DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
}

// Render on-screen text each frame
void RenderSceneText( float updateTime )
{
	// Accumulate update times to calculate the average over a given period
	SumUpdateTimes += updateTime;
	++NumUpdateTimes;
	if (SumUpdateTimes >= UpdateTimePeriod)
	{
		AverageUpdateTime = SumUpdateTimes / NumUpdateTimes;
		SumUpdateTimes = 0.0f;
		NumUpdateTimes = 0;
	}

	// Write FPS text string
	stringstream outText;
	if (AverageUpdateTime >= 0.0f)
	{
		outText << "Frame Time: " << AverageUpdateTime * 1000.0f << "ms" << endl << "FPS:" << 1.0f / AverageUpdateTime;
		RenderText( outText.str(), 2, 2, 0.0f, 0.0f, 0.0f );
		RenderText( outText.str(), 0, 0, 1.0f, 1.0f, 0.0f );
		outText.str("");
	}

	//Picking	
	if (!ChaseCamera)
	{
		if (!GrabbedTank)
		{
			CVector2 MousePixel = { TFloat32(MouseX),TFloat32(MouseY) };

			//For each tank, check distance from mouse pixel
			EntityManager.BeginEnumEntities("", "", "Tank");
			CEntity* entity;
			while (entity = EntityManager.EnumEntity())
			{
				if (EntityManager.GetEntity(NearestTankEntity) == 0)
				{
					NearestTankEntity = 0;
				}
				if (NearestTankEntity == 0)
				{
					NearestEntityDistance = INT_MAX;
				}
				else
				{
					//Get nearest tank pixel from world
					TInt32 x, y = 0;
					MainCamera->PixelFromWorldPt(EntityManager.GetEntity(NearestTankEntity)->Position(), ViewportWidth, ViewportHeight, &x, &y);
					CVector2 nearestEntityPos2D = { TFloat32(x),TFloat32(y) };

					//Store distance from nearest tank to pixel
					NearestEntityDistance = Distance(MousePixel, nearestEntityPos2D);
				}

				//Find tank pixel from world
				TInt32 x, y = 0;
				MainCamera->PixelFromWorldPt(entity->Position(), ViewportWidth, ViewportHeight, &x, &y);
				CVector2 entityPos2D = { TFloat32(x),TFloat32(y) };

				//Check if distance is smaller than the current nearest distance found
				auto currentEntityDistance = Distance(MousePixel, entityPos2D);
				if (currentEntityDistance < NearestEntityDistance)
				{
					//Update the nearest tank entity
					NearestTankEntity = entity->GetUID();
				}
			}
			EntityManager.EndEnumEntities();
		}
	}

	//For each tank, render text
	EntityManager.BeginEnumEntities("", "", "Tank");
	CEntity* entity = 0;
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* tankEntity = dynamic_cast<CTankEntity*>(EntityManager.GetEntity(entity->GetUID()));
		if (tankEntity)
		{
			//If in front of the camera
			int X, Y = 0;
			if (MainCamera->PixelFromWorldPt(tankEntity->Position(), ViewportWidth, ViewportHeight, &X, &Y))
			{
				//Render template name and entity name
				outText << tankEntity->Template()->GetName().c_str() << " " << tankEntity->GetName().c_str();

				//If extended info turned on 
				if (ToggleExtendedInfo)
				{
					//Render health, state and shellcount
					outText << " " << tankEntity->GetHP() << " "
						<< tankEntity->GetState() << " "
						<< tankEntity->GetShellCount();
				}

				//If nearest tank to cursor
				if (tankEntity->GetUID() == NearestTankEntity)
				{ 
					//If grabbed render text red else render text yellow
					if (!GrabbedTank) { RenderText(outText.str(), X, Y, 1.0f, 1.0f, 0.0f, true); }
					else { RenderText(outText.str(), X, Y, 1.0f, 0.0f, 0.0f, true); }
				}

				//Render team 0 green
				else if(tankEntity->GetTeam() == 0) { RenderText(outText.str(), X, Y, 0.0f, 1.0f, 0.0f, true); }

				//Render team 1 blue
				else if (tankEntity->GetTeam() == 1) { RenderText(outText.str(), X, Y, 0.0f, 0.0f, 1.0f, true); }

				outText.str("");
			}
		}
	}
	EntityManager.EndEnumEntities();

}


// Update the scene between rendering
void UpdateScene( float updateTime )
{
	// Call all entity update functions
	EntityManager.UpdateAllEntities( updateTime );

	//Get pointer to nearest entity
	CEntity* nearestEntity = EntityManager.GetEntity(NearestTankEntity);

	// Set camera speeds
	// Key F1 used for full screen toggle
	if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
	if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;
	if (KeyHit(Key_1))//Send start message to all tanks
	{
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = 0;
		while (entity = EntityManager.EnumEntity())
		{
			SMessage msg;
			msg.type = Msg_Start;
			Messenger.SendMessageA(entity->GetUID(), msg);
			AmmoTimerStarted = false;
		}

	}
	if (KeyHit(Key_2)) //Send stop message to all tanks
	{
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = 0;
		while (entity = EntityManager.EnumEntity())
		{
			SMessage msg;
			msg.type = Msg_Stop;
			Messenger.SendMessageA(entity->GetUID(), msg);
		}
	}
	if (KeyHit(Key_0)) //Toggle extended info text under tank
	{
		if (ToggleExtendedInfo)
		{
			ToggleExtendedInfo = false;
		}
		else
		{
			ToggleExtendedInfo = true;
		}
	}
	if (KeyHit(Mouse_LButton)) 
	{
		//If tank has been grabbed
		if (GrabbedTank)
		{
			//Calculate new position for tank
			auto worldPt = MainCamera->WorldPtFromPixel(MouseX, MouseY, ViewportWidth, ViewportHeight);
			auto direction = worldPt - MainCamera->Position();
			direction.Normalise();
			auto newPosition = worldPt + PickDist * direction;

			//Set new position
			nearestEntity->Position().x = newPosition.x;
			nearestEntity->Position().z = newPosition.z;

			//Let go of the tank
			GrabbedTank = false;
		}
		else
		{
			//Grab the tank
			GrabbedTank = true;
		}

		//Send a start message to the tank
		SMessage msg;
		msg.type = Msg_Start;
		Messenger.SendMessageA(nearestEntity->GetUID(), msg);

	}
	if (KeyHit(Mouse_RButton)) //Toggle chase camera for nearest tank to cursor
	{
		if (ChaseCamera)
		{
			ChaseCamera = false;
		}
		else
		{
			ChaseCamera = true;
		}
	}

	if (ChaseCamera)
	{
		// Take camera position from player, moved backwards and upwards
		MainCamera->Position() = nearestEntity->Position() - nearestEntity->Matrix().ZAxis() * 12.0f +
			nearestEntity->Matrix().YAxis() * 5.0f;
		// Face camera towards point above player
		MainCamera->Matrix().FaceTarget(nearestEntity->Position() + CVector3(0, 3.0f, 0));
	}
	else
	{
		// Move the camera
		MainCamera->Control(Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D,
			CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime);
	}

	if (!AmmoTimerStarted)
	{
		//Count down a random length timer to deploy ammo for tanks
		AmmoTimer.Start();
		AmmoTimerDuration = Random(5, 15);
		AmmoTimerStarted = true;
	}

	if (AmmoTimer.GetTime() > AmmoTimerDuration)
	{
		//Reset the ammo timer
		AmmoTimer.Reset();
		AmmoTimerStarted = false;
		AmmoTimerDuration = 0;

		//Spawn a new ammo crate
		auto newAmmoUID = EntityManager.CreateAmmo("Ammo", "Ammo", CVector3(Random(-100.0f, 100.0f), 50.0f, Random(-100.0f, 100.0f)));
		EntityManager.GetEntity(newAmmoUID)->Matrix().Scale({ 0.5,0.5,0.5 });
	}

}


} // namespace gen
