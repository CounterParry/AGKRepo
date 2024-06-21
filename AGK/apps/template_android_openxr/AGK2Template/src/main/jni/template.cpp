 // Includes
#include <string>
#include <sstream>
#include "template.h"
#include "agkopenxr.h"

// Namespace
using namespace AGK;

// OpenXR Commands: Just the basics. Openxr supports many other features then this, so more commands can be added...
namespace agkopenxr
{
	// Setup
	void  SetOpenXRScale(float Scale);
	float GetOpenXRScale();
	void  SetCameraRange(float Near, float Far);
	void  SetFollowHMDLook(int Look);
	void  SetFollowHMDY(int Setting);

	// Movement
	void SetPosition(float X, float Y, float Z);
	void SetRotation(float X, float Y, float Z);
	void MoveX(float Distance);
	void MoveY(float Distance);
	void MoveZ(float Distance);
	void RotateX(float Amount);
	void RotateY(float Amount);
	void RotateZ(float Amount);

    // Player
	float GetX();
	float GetY_Head();
	float GetY_Feet();
	float GetZ();
	float GetQuatW();
	float GetQuatX();
	float GetQuatY();
	float GetQuatZ();
	float GetAngleX();
	float GetAngleY();
	float GetAngleZ();

	// Right Hand
	int   RightExists();
	void  GetRight(float *X, float *Y, float *Z, float *QuatW, float *QuatX, float *QuatY, float *QuatZ);
	float GetRightX();
	float GetRightY();
	float GetRightZ();
	float GetRightAngleX();
	float GetRightAngleY();
	float GetRightAngleZ();
	float GetRightQuatW();
	float GetRightQuatX();
	float GetRightQuatY();
	float GetRightQuatZ();
	bool  GetRightButtonAPressed();
    bool  GetRightButtonBPressed();
    bool  GetRightButtonGripPressed();
    bool  GetRightButtonThumbstickClickPressed();
    float GetRightTrigger();
    void  GetRightThumbstick(float *X, float *Y);
	void  SetRightHaptic(float Amount);

	// Left Hand
	int   LeftExists();
	void  GetLeft(float *X, float *Y, float *Z, float *QuatW, float *QuatX, float *QuatY, float *QuatZ); 
	float GetLeftX();
	float GetLeftY();
	float GetLeftZ();
	float GetLeftAngleX();
	float GetLeftAngleY();
	float GetLeftAngleZ();
	float GetLeftQuatW();
	float GetLeftQuatX();
	float GetLeftQuatY();
	float GetLeftQuatZ();
    bool  GetLeftButtonXPressed();
    bool  GetLeftButtonYPressed();
    bool  GetLeftButtonGripPressed();
    bool  GetLeftButtonThumbstickClickPressed();
    float GetLeftTrigger();
    void  GetLeftThumbstick(float *X, float *Y);
	void  SetLeftHaptic(float Amount);

	// Information
	std::string GetSystemName();
	std::string GetSytemTracking();
	std::string GetSystemID();
	std::string GetVendorID();

	// Basic
	int   Begin(int ObjectID, int ScreenIMG);
	void  UpdateOpenXR();
    void  Sync();
    void  End();
};

app App;

// Lets Display Something...
bool bButton = false;
const int   Number_Of_Boxes   = 6;
const float Box_Distance    = 2.5f;
const float Box_Size        = 0.5f;
int   iBox[Number_Of_Boxes];
int   iPlane;
float iBoxX = 0;
float iBoxY = 0;
float iBoxZ = 0;
int iLeft, iRight; // VR Controllers
int iLeft1, iRight1; // VR Controllers
int iTelevision;
int iHMD;

float iPlayerX,  iPlayerY,  iPlayerZ;
float iPlayerAX, iPlayerAY, iPlayerAZ;

// VR Buttons (Left X button and Right A button will return haptic feedback.)
void buttons()
{
	float iLeftX, iLeftY;
	float iRightX, iRightY;
	agkopenxr::GetLeftThumbstick(&iLeftX, &iLeftY);
	agkopenxr::GetRightThumbstick(&iRightX, &iRightY);

	if (agkopenxr::GetLeftButtonXPressed())
	{
		if (bButton == false)
		{
			agkopenxr::SetRotation(0, 45.0f, 0);
		}
		bButton = true;
	} 
    else if (agkopenxr::GetLeftButtonYPressed())
	{
		if (bButton == false)
		{
			agkopenxr::SetPosition(-2.75, 0, -2.75);
		}
		bButton = true;
	}
    else if (agkopenxr::GetLeftButtonGripPressed())
	{
		bButton = true;
	} 
    else if (agkopenxr::GetLeftButtonThumbstickClickPressed())
	{
		bButton = true;
	}
    else if (agkopenxr::GetLeftTrigger() > 0.5f)
	{
		bButton = true;
	} 
	else if (agkopenxr::GetRightButtonAPressed())
	{
		if (bButton == false)
		{
			agkopenxr::SetRightHaptic(2.0f);
		}
		bButton = true;
	}
    else if (agkopenxr::GetRightButtonBPressed())
	{
		bButton = true;
	}
    else if (agkopenxr::GetRightButtonGripPressed())
	{
		 bButton = true;
	}
    else if (agkopenxr::GetRightButtonThumbstickClickPressed())
	{
		bButton = true;
	}
    else if (agkopenxr::GetRightTrigger() > 0.5f)
	{
		 bButton = true;
	}
	else if (iLeftY < -0.75f || iRightY < -0.75f)
	{
		if (bButton == false) {}

		agkopenxr::MoveZ(-0.1f);

		bButton = true;
	}
	else if (iLeftY > 0.75f || iRightY > 0.75f)
	{
		if (bButton == false) {}

		agkopenxr::MoveZ(0.1f); // Z is used in FFVR

		bButton = true;
	}
	else if (iLeftX < -0.75f || iRightX < -0.75f)
	{
		if (bButton == false) {}

		agkopenxr::RotateY(-1); // Y is used in FFVR

		bButton = true;
	}
	else if (iLeftX > 0.75f || iRightX > 0.75f)
	{
		if (bButton == false) {}

		agkopenxr::RotateY(1);

		bButton = true;
	}
	else
	{
		bButton = false;
	}
}

void app::Begin(void)
{
	LOGI("AGK BEGIN: Start");

	agkopenxr::Begin(9000, 9000); // Setup OpenXR...

    agk::SetClearColor( 0,0,100 ); // Blue skys
	
	// Display VR Controllers
	iLeft  = agk::CreateObjectSphere(0.1f, 10, 10);
	iRight = agk::CreateObjectSphere(0.1f, 10, 10);
	iLeft1  = agk::CreateObjectSphere(0.11f, 3, 3);
	iRight1 = agk::CreateObjectSphere(0.11f, 3, 3);

	// Slow Spinning Boxes
	for (int a = 0; a < Number_Of_Boxes; a++)
	{
		iBox[a] = agk::CreateObjectBox(Box_Size, Box_Size, Box_Size);

		if (a == 0)
		{
			agk::SetObjectPosition(iBox[a], 0, -Box_Distance, 0);
			agk::SetObjectColor(iBox[a], 136, 139, 141, 125); // Gray
		} // - Y

		if (a == 1)
		{
			agk::SetObjectPosition(iBox[a], 0, Box_Distance, 0);
			agk::SetObjectColor(iBox[a], 225, 164, 0, 125); // Orange
		} // + Y

		if (a == 2)
		{
			agk::SetObjectPosition(iBox[a], 0, 0, -Box_Distance);
			agk::SetObjectColor(iBox[a], 0, 0, 255, 125); // Blue
		} // - Z

		if (a == 3)
		{
			agk::SetObjectPosition(iBox[a], 0, 0, Box_Distance);
			agk::SetObjectColor(iBox[a], 255, 0, 0, 125); // Red
		} // + Z

		if (a == 4)
		{
			agk::SetObjectPosition(iBox[a], -Box_Distance, 0, 0);
			agk::SetObjectColor(iBox[a], 0, 255, 0, 255); // Green
		} // - X

		if (a == 5)
		{
			agk::SetObjectPosition(iBox[a], Box_Distance, 0, 0);
			agk::SetObjectColor(iBox[a], 255, 255, 0, 125); // Yellow
		} // + X
	}

	// Lets add a Plain
	iPlane = agk::CreateObjectPlane(75, 75);
	agk::SetObjectPosition(iPlane, 0, -10, 0);
	agk::SetObjectRotation(iPlane, 90, 90, 0);
	agk::SetObjectColor(iPlane, 255, 0, 0, 255); // Red

	// TV 
	iTelevision = agk::CreateObjectPlane(0.1920f * 3, 0.1080f * 3);
	agk::SetObjectImage(iTelevision, 9000, 0);
	agk::SetObjectPosition(iTelevision, 0, Box_Size, 0);
	agk::SetObjectRotation(iTelevision, 0, 45, 0);

	// HMD
	iHMD = agk::CreateObjectSphere(Box_Size * 0.2f, 10, 10);
	agk::SetObjectColor(iHMD, 0, 255, 0, 255); // Green

	// Line of Spheres
	for (int a = 0; a < 200; a++)
	{
		int iSphere = agk::CreateObjectSphere(0.025f, 10, 10);
		agk::SetObjectPosition(iSphere, 0, 0, float(a) * 0.5f);
	}

	// Set Player Starting Position
	iPlayerX  = 0;
	iPlayerY  = 0;
	iPlayerZ  = 0;
	iPlayerAX = 0;
	iPlayerAY = 0;
	iPlayerAZ = 0;
	//agkopenxr::SetPosition(iPlayerX, iPlayerY, iPlayerZ);
	//agkopenxr::SetRotation(iPlayerAX, iPlayerAY, iPlayerAZ);

	// Lock Pitch
	agkopenxr::SetFollowHMDY(0); // 0
	agkopenxr::SetFollowHMDLook(1); // 1
	LOGI("AGK BEGIN: End");
}
int  app::Loop (void)
{
	//LOGI("AGK LOOP: Start");

	// Slowly Spin The Cubes
	iBoxX = iBoxX + .01f;
	iBoxY = iBoxY + .02f;
	iBoxZ = iBoxZ + .03f;
	if (iBoxX > 360) iBoxX = iBoxX - 360;
	if (iBoxY > 360) iBoxY = iBoxY - 360;
	if (iBoxZ > 360) iBoxZ = iBoxZ - 360;
	for (int a = 0; a < Number_Of_Boxes; a++)
	{
		agk::SetObjectRotation(iBox[a], iBoxX, iBoxY, iBoxZ);
	}
	
	buttons();

	agkopenxr::UpdateOpenXR();

	iPlayerX  = agkopenxr::GetX();
	iPlayerY  = agkopenxr::GetY_Feet();
	iPlayerZ  = agkopenxr::GetZ();
	iPlayerAX = agkopenxr::GetAngleX();
	iPlayerAY = agkopenxr::GetAngleY();
	iPlayerAZ =	agkopenxr::GetAngleZ();

	// Update Hands Positions
	float iX, iY, iZ, iQuatW, iQuatX, iQuatY, iQuatZ;
	agkopenxr::GetLeft(&iX, &iY, &iZ, &iQuatW, &iQuatX, &iQuatY, &iQuatZ);
	agk::SetObjectPosition(iLeft, iX, iY, iZ);
	agk::SetObjectRotationQuat(iLeft, iQuatW, iQuatX, iQuatY, iQuatZ);
	agkopenxr::GetRight(&iX, &iY, &iZ, &iQuatW, &iQuatX, &iQuatY, &iQuatZ);
	agk::SetObjectPosition(iRight, iX, iY, iZ);
	agk::SetObjectRotationQuat(iRight, iQuatW, iQuatX, iQuatY, iQuatZ);

	// Update TV
	agk::SetObjectPosition(iTelevision, iPlayerX + 0.4f, Box_Size, iPlayerZ + 0.4f);

	// Update HMD
	float iHMDX  = agkopenxr::GetX();
	float iHMDY  = agkopenxr::GetY_Head();
	float iHMDZ  = agkopenxr::GetZ();
	float iHMDAX = agkopenxr::GetAngleX();
	float iHMDAY = agkopenxr::GetAngleY();
	float iHMDAZ = agkopenxr::GetAngleZ();
	agk::SetObjectPosition(iHMD, iHMDX, iHMDY, iHMDZ);
	agk::SetObjectRotation(iHMD, iHMDAX, iHMDAY, iHMDAZ);

	iX = agkopenxr::GetLeftX();
	iY = agkopenxr::GetLeftY();
	iZ = agkopenxr::GetLeftZ();
	agk::SetObjectPosition(iLeft1, iX, iY, iZ);
	iX = agkopenxr::GetLeftAngleX();
	iY = agkopenxr::GetLeftAngleY();
	iZ = agkopenxr::GetLeftAngleZ();
	agk::SetObjectRotation(iLeft1, iX, iY, iZ);

	iX = agkopenxr::GetRightX();
	iY = agkopenxr::GetRightY();
	iZ = agkopenxr::GetRightZ();
	agk::SetObjectPosition(iRight1, iX, iY, iZ);
	iX = agkopenxr::GetRightAngleX();
	iY = agkopenxr::GetRightAngleY();
	iZ = agkopenxr::GetRightAngleZ();
	agk::SetObjectRotation(iRight1, iX, iY, iZ);

	// Find Frames Per Second
	std::stringstream sStr;
	sStr << "FPS:" << agk::ScreenFPS() 
	     << " Frametime:" << agk::GetFrameTime()
	     << " Timer:" << agk::Timer();
	std::string sString = sStr.str();
	agk::Print(sString.c_str());

	// Player Location
	sStr.str("");
	sStr << "Player X:" << iPlayerX << 
			" Y:" << iPlayerY <<
			" Z:" << iPlayerZ;
	sString = sStr.str();
	agk::Print(sString.c_str());

	// Player Rotation
	sStr.str("");
	sStr << "Player Xa:" << iPlayerAX << 
			" Ya:" << iPlayerAY <<
			" Za:" << iPlayerAZ ;
	sString = sStr.str();
	agk::Print(sString.c_str());

	agkopenxr::Sync();
	//agk::Sync(); // No need, this is called inside of agkopenxr::Sync();

	LOGI("AGK LOOP: End");
	return 0; // return 1 to close app
}
void app::End (void)
{
	LOGI("AGK END: Start");
	agkopenxr::End();
	LOGI("AGK END: End");
}
