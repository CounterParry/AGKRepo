// Includes
#include <string>
#include "template.h"
#include "agkopenxr.h"

// Namespace
using namespace AGK;

// OpenXR Commands: Just the basics. Openxr supports many other features then this, so more commands should be added...
namespace agkopenxr
{
	void  GetLeftHand(float *X, float *Y, float *Z, float *QuatW, float *QuatX, float *QuatY, float *QuatZ); 
	void  GetRightHand(float *X, float *Y, float *Z, float *QuatW, float *QuatX, float *QuatY, float *QuatZ);
    bool  GetLeftHandButtonXPressed();
    bool  GetLeftHandButtonYPressed();
    bool  GetLeftHandButtonGripPressed();
    bool  GetLeftHandButtonThumbstickClickPressed();
    float GetLeftHandTrigger();
    void  GetLeftHandThumbstick(float *X, float *Y);
	void  SetLeftHandBuzz(float Amount); // Haptic Feedback
    bool  GetRightHandButtonAPressed();
    bool  GetRightHandButtonBPressed();
    bool  GetRightHandButtonGripPressed();
    bool  GetRightHandButtonThumbstickClickPressed();
    float GetRightHandTrigger();
    void  GetRightHandThumbstick(float *X, float *Y);
	void  SetRightHandBuzz(float Amount); // Haptic Feedback
    void  Begin(); // Starts OpenXR
    void  Sync(); // Syncs VR 
    void  End(); // Shut down OpenXR
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

// VR Buttons (Left X button and Right A button will return haptic feedback.)
void buttons()
{
	float iLeftX, iLeftY;
	float iRightX, iRightY;
	agkopenxr::GetLeftHandThumbstick(&iLeftX, &iLeftY);
	agkopenxr::GetRightHandThumbstick(&iRightX, &iRightY);

	if (agkopenxr::GetLeftHandButtonXPressed())
	{
		if (bButton == false) {agkopenxr::SetLeftHandBuzz(2.0f);}
		bButton = true;
	} 
    else if (agkopenxr::GetLeftHandButtonYPressed())
	{
		if (bButton == false){}
		bButton = true;
	}
    else if (agkopenxr::GetLeftHandButtonGripPressed())
	{
		bButton = true;
	} 
    else if (agkopenxr::GetLeftHandButtonThumbstickClickPressed())
	{
		bButton = true;
	}
    else if (agkopenxr::GetLeftHandTrigger() > 0.5f)
	{
		bButton = true;
	} 
	else if (agkopenxr::GetRightHandButtonAPressed())
	{
		if (bButton == false) {agkopenxr::SetRightHandBuzz(2.0f);}
		bButton = true;
	}
    else if (agkopenxr::GetRightHandButtonBPressed())
	{
		bButton = true;
	}
    else if (agkopenxr::GetRightHandButtonGripPressed())
	{
		 bButton = true;
	}
    else if (agkopenxr::GetRightHandButtonThumbstickClickPressed())
	{
		bButton = true;
	}
    else if (agkopenxr::GetRightHandTrigger() > 0.5f)
	{
		 bButton = true;
	}
	else if (iLeftX < -0.75f || iRightX < -0.75f)
	{
		if (bButton == false) {}
		bButton = true;
	}
	else if (iLeftX > 0.75f || iRightX > 0.75f)
	{
		if (bButton == false) {}
		bButton = true;
	}
	else if (iLeftY < -0.75f || iRightY < -0.75f)
	{
		if (bButton == false) {}
		bButton = true;
	}
	else if (iLeftY > 0.75f || iRightY > 0.75f)
	{
		if (bButton == false) {}
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

	agkopenxr::Begin(); // Setup OpenXR...

    agk::SetClearColor( 0,0,100 ); // Blue skys
	
	// Display VR Controllers
	iLeft  = agk::CreateObjectSphere(0.1f, 10, 10);
	iRight = agk::CreateObjectSphere(0.1f, 10, 10);

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

	// Line of Spheres
	for (int a = 0; a < 200; a++)
	{
		int iSphere = agk::CreateObjectSphere(0.025f, 10, 10);
		agk::SetObjectPosition(iSphere, 0, 0, float(a) * 0.5f);
	}

	LOGI("AGK BEGIN: End");
}
int  app::Loop (void)
{
	LOGI("AGK LOOP: Start");

	// Display AGK Errors
	uString err;
	agk::GetLastError(err);
	err.Prepend( "Error: " );
	std::string sErr = err.GetStr();

	//for (int a = 0; a < 1000; a++) agk::Print("********************************************************************"); // This doesn't work =(

	// Slowly Spin The Cubes
	iBoxX = iBoxX + .001f;
	iBoxY = iBoxY + .002f;
	iBoxZ = iBoxZ + .003f;
	if (iBoxX > 360) iBoxX = iBoxX - 360;
	if (iBoxY > 360) iBoxY = iBoxY - 360;
	if (iBoxZ > 360) iBoxZ = iBoxZ - 360;
	for (int a = 0; a < Number_Of_Boxes; a++) agk::SetObjectRotation(iBox[a], iBoxX, iBoxY, iBoxZ);

	// Update Hands Positions
	float iX, iY, iZ, iQuatW, iQuatX, iQuatY, iQuatZ;
	agkopenxr::GetLeftHand(&iX, &iY, &iZ, &iQuatW, &iQuatX, &iQuatY, &iQuatZ);
	agk::SetObjectPosition(iLeft, iX, iY, iZ);
	agk::SetObjectRotationQuat(iLeft, iQuatW, iQuatX, iQuatY, iQuatZ);
	agkopenxr::GetRightHand(&iX, &iY, &iZ, &iQuatW, &iQuatX, &iQuatY, &iQuatZ);
	agk::SetObjectPosition(iRight, iX, iY, iZ);
	agk::SetObjectRotationQuat(iRight, iQuatW, iQuatX, iQuatY, iQuatZ);

	buttons();

	agkopenxr::Sync();
	//agk::Sync(); // Doesn't Do Anything...

	LOGI("AGK LOOP: End");
	return 0; // return 1 to close app
}
void app::End (void)
{
	LOGI("AGK END: Start");
	agkopenxr::End();
	LOGI("AGK END: End");
}
