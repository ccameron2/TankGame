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

// Tank UIDs
TEntityUID TankA;
TEntityUID TankB;
TEntityUID TankC;
TEntityUID TankD;
TEntityUID TankE;
TEntityUID TankF;

// Other scene elements
const int NumLights = 2;
CLight*  Lights[NumLights];
SColourRGBA AmbientLight;
CCamera* MainCamera;

// Sum of recent update times and number of times in the sum - used to calculate
// average over a given time period
float SumUpdateTimes = 0.0f;
int NumUpdateTimes = 0;
float AverageUpdateTime = -1.0f; // Invalid value at first

//Picking
CEntity* NearestEntity;
float NearestEntityDistance;

bool toggleExtendedInfo = true;
bool grabbedTank = false;
int pickDist = 50;

//Chase camera
bool chaseCamera = false;
CEntity* chasedTank;

//The program insists on clicking once when run?
bool whyisthishappening = false;

//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	//////////////////////////////////////////////
	// Prepare render methods

	InitialiseMethods();

	LevelParser.ParseFile("Entities.xml");

	EntityManager.BeginEnumEntities("Tree", "Tree", "Scenery");
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
TEntityUID GetTankUID(int team)
{
	return (team == 0) ? TankA : TankB;
}


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
	
	
	if (!chaseCamera)
	{
		if (!grabbedTank)
		{
			CVector2 MousePixel = { TFloat32(MouseX),TFloat32(MouseY) };

			EntityManager.BeginEnumEntities("", "", "Tank");
			CEntity* entity;
			while (entity = EntityManager.EnumEntity())
			{
				if (NearestEntity == 0)
				{
					NearestEntityDistance = 9999999;
				}
				else
				{
					if (NearestEntity)
					{
						TInt32 x, y = 0;
						MainCamera->PixelFromWorldPt(NearestEntity->Position(), ViewportWidth, ViewportHeight, &x, &y);
						CVector2 nearestEntityPos2D = { TFloat32(x),TFloat32(y) };
						NearestEntityDistance = Distance(MousePixel, nearestEntityPos2D);
					}
				}
				TInt32 x, y = 0;
				MainCamera->PixelFromWorldPt(entity->Position(), ViewportWidth, ViewportHeight, &x, &y);
				CVector2 entityPos2D = { TFloat32(x),TFloat32(y) };
				auto currentEntityDistance = Distance(MousePixel, entityPos2D);
				if (currentEntityDistance < NearestEntityDistance)
				{
					NearestEntity = entity;
				}
			}
			EntityManager.EndEnumEntities();
		}
	}

	EntityManager.BeginEnumEntities("", "", "Tank");
	CEntity* entity = 0;
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* tankEntity = dynamic_cast<CTankEntity*>(EntityManager.GetEntity(entity->GetUID()));
		if (tankEntity)
		{
			int X, Y = 0;
			if (MainCamera->PixelFromWorldPt(tankEntity->Position(), ViewportWidth, ViewportHeight, &X, &Y))
			{
				outText << tankEntity->Template()->GetName().c_str() << " " << tankEntity->GetName().c_str();
				if (toggleExtendedInfo)
				{
					outText << " " << tankEntity->GetHP() << " "
						<< tankEntity->GetState() << " "
						<< tankEntity->GetShellCount();
				}
				if (tankEntity == NearestEntity) 
				{ 
					if (!grabbedTank) { RenderText(outText.str(), X, Y, 1.0f, 1.0f, 0.0f, true); }
					else { RenderText(outText.str(), X, Y, 1.0f, 0.0f, 0.0f, true); }
				}
				else if(tankEntity->GetTeam() == 0) { RenderText(outText.str(), X, Y, 0.0f, 1.0f, 0.0f, true); }
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

	// Set camera speeds
	// Key F1 used for full screen toggle
	if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
	if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;
	if (KeyHit(Key_1))
	{
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = 0;
		while (entity = EntityManager.EnumEntity())
		{
			SMessage msg;
			msg.type = Msg_Start;
			Messenger.SendMessageA(entity->GetUID(), msg);
		}

	}
	if (KeyHit(Key_2))
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
	if (KeyHit(Key_0))
	{
		if (toggleExtendedInfo)
		{
			toggleExtendedInfo = false;
		}
		else
		{
			toggleExtendedInfo = true;
		}
	}
	if (KeyHit(Mouse_LButton))
	{
		if (!whyisthishappening)
		{
			whyisthishappening = true;
		}
		else
		{
			if (grabbedTank)
			{
				auto worldPt = MainCamera->WorldPtFromPixel(MouseX, MouseY, ViewportWidth, ViewportHeight);
				auto direction = worldPt - MainCamera->Position();
				direction.Normalise();
				auto newPosition = worldPt + pickDist * direction;
				NearestEntity->Position().x = newPosition.x;
				NearestEntity->Position().z = newPosition.z;
				grabbedTank = false;
			}
			else
			{
				grabbedTank = true;
			}
			SMessage msg;
			msg.type = Msg_Start;
			Messenger.SendMessageA(NearestEntity->GetUID(), msg);
		}
	}
	if (KeyHit(Mouse_RButton))
	{
		if (chaseCamera)
		{
			chaseCamera = false;
		}
		else
		{
			chaseCamera = true;
		}
	}

	if (chaseCamera)
	{
		// Take camera position from player, moved backwards and upwards
		MainCamera->Position() = NearestEntity->Position() - NearestEntity->Matrix().ZAxis() * 12.0f +
			NearestEntity->Matrix().YAxis() * 5.0f;
		// Face camera towards point above player
		MainCamera->Matrix().FaceTarget(NearestEntity->Position() + CVector3(0, 3.0f, 0));
	}
	else
	{
		// Move the camera
		MainCamera->Control(Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D,
			CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime);
	}

}


} // namespace gen
