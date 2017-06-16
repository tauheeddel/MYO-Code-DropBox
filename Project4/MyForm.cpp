#include "MyForm.h"
#include <myo/myo.hpp>
#include<stdio.h>
#include<winsock2.h>
#include<WS2tcpip.h>

//added by Tauheed


using namespace System;
using namespace System::IO::Ports;
#pragma comment(lib,"ws2_32.lib") //Winsock Library
#define DEFAULT_BUFLEN 512
#define MAX_THREADS 2
// Copyright (C) 2013-2014 Thalmic Labs Inc.
// Distributed under the Myo SDK license agreement. See LICENSE.txt for details.
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <windows.h>
#include <strsafe.h>


DWORD WINAPI MyThreadFunction(LPVOID lpParam);
void ErrorHandler(LPTSTR lpszFunction);
typedef struct MyData {
	int *val1;

} MYDATA, *PMYDATA;
using namespace System;
using namespace System::IO::Ports;
//#pragma comment(lib,"ws2_32.lib") 
//#define DEFAULT_BUFLEN 512
//#define _USE_MATH_DEFINES


class DataCollector : public myo::DeviceListener {
public:
	DataCollector()
		: onArm(false), roll_w(0), pitch_w(0), yaw_w(0), currentPose()
	{
	}

	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		roll_w = 0;
		pitch_w = 0;
		yaw_w = 0;
		onArm = false;
	}

	void onOrientationData(myo::Myo* myo, uint64_t timestamp, const myo::Quaternion<float>& quat)
	{
		using std::atan2;
		using std::asin;
		using std::sqrt;
		using std::max;
		using std::min;

		float roll = atan2(2.0f * (quat.w() * quat.x() + quat.y() * quat.z()),
			1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y()));
		float pitch = asin(max(-1.0f, min(1.0f, 2.0f * (quat.w() * quat.y() - quat.z() * quat.x()))));
		float yaw = atan2(2.0f * (quat.w() * quat.z() + quat.x() * quat.y()),
			1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z()));

		roll_w = static_cast<int>((roll + (float)3.14) / (3.14 * 2.0f) * 18);
		pitch_w = static_cast<int>((pitch + (float)3.14 / 2.0f) / 3.14 * 18);
		yaw_w = static_cast<int>((yaw + (float)3.14) / (3.14 * 2.0f) * 18);
	}

	void onPose(myo::Myo* myo, uint64_t timestamp, myo::Pose pose)
	{
		currentPose = pose;

	}

	void onArmSync(myo::Myo* myo, uint64_t timestamp, myo::Arm arm, myo::XDirection xDirection)
	{
		onArm = true;
		whichArm = arm;
	}

	void onArmUnsync(myo::Myo* myo, uint64_t timestamp)
	{
		onArm = false;
	}

	void print()
	{


		std::cout << '\r';

		if (onArm) {
			std::string poseString = currentPose.toString();

			std::cout << '[' << (whichArm == myo::armLeft ? "L" : "R") << ']'
				<< '[' << poseString << std::string(14 - poseString.size(), ' ') << ']';
		}
		else {
			std::cout << "[?]" << '[' << std::string(14, ' ') << ']';
		}

		std::cout << std::flush;
	}

	bool onArm;
	myo::Arm whichArm;

	int roll_w, pitch_w, yaw_w;
	myo::Pose currentPose;
};

int recvbuflen = DEFAULT_BUFLEN;
char recvbuf[DEFAULT_BUFLEN];

DWORD WINAPI MyThreadFunction1(LPVOID lpParam)
{
	HANDLE hStdout;
	PMYDATA pDataArray;

	WSADATA wsa;
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	int iResult;
	int iSendResult;
	
	
	ZeroMemory(recvbuf, recvbuflen);

	printf("\nInitialising Winsock...\n");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, "12345", &hints, &result) != 0) {
		printf("getaddrinfo failed with error.\n");
		WSACleanup();
		return 1;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	if (bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("Connection accepted\n");

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdout == INVALID_HANDLE_VALUE)
		return 1;

	pDataArray = (PMYDATA)lpParam;
	while (1) {
		//int *point = pDataArray->val1;
		//*point = *point + 2;

		//printf("Thread 1 through speech recvbuf %s\n",recvbuf);
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		
		if (iResult > 0) {
			recvbuf[iResult] = '\0';
			printf("Command received: %s\n", recvbuf);
			//ZeroMemory(recvbuf, recvbuflen);
		}
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
		//Sleep(1000);
	}
	return 0;
}

DWORD WINAPI MyThreadFunction2(LPVOID lpParam)
{
	HANDLE hStdout;
	PMYDATA pDataArray;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdout == INVALID_HANDLE_VALUE)
		return 1;

	System::IO::Ports::SerialPort^ SerialPortToDevice;
	SerialPortToDevice = gcnew System::IO::Ports::SerialPort();

	array<String^>^ serialPorts = nullptr;
	serialPorts = SerialPortToDevice->GetPortNames();
	Console::WriteLine("Available Ports:");
	for each(String^ port in serialPorts)
	{
		Console::WriteLine(port);
	}
	SerialPortToDevice->PortName = "COM6";
	SerialPortToDevice->BaudRate = 9600;
	SerialPortToDevice->Open();

	pDataArray = (PMYDATA)lpParam;
	//while (1) {
		int *point = pDataArray->val1;
		*point = *point + 2;

		try {

			myo::Hub hub("com.jakechapeskie.SerialCommunication");

			std::cout << "Attempting to find a Myo..." << std::endl;
			myo::Myo* myo = hub.waitForMyo(10000);

			if (!myo) {
				throw std::runtime_error("Unable to find a Myo!");
			}

			std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

			DataCollector collector;

			hub.addListener(&collector);

			hub.setLockingPolicy(myo::Hub::LockingPolicy::lockingPolicyStandard);
			while (1) {
				hub.run(1000 / 20);
				collector.print();
				
				//printf("recvbuf Thread 2  %s\n", recvbuf);
				if (strcmp(recvbuf, "Move Right") == 0)
				{
					printf("We are here %s\n", recvbuf);
					//std::string poseString = (collector.currentPose.toString());
					
					//String^ poseStrorageString = gcnew String(poseString.c_str());
					if (SerialPortToDevice->IsOpen) {
						printf("We are here %s\n", recvbuf);
						for (int i = 0; i < 3; i++){
							SerialPortToDevice->WriteLine("waveOut");
							Sleep(300);
						}
					}
					ZeroMemory(recvbuf, recvbuflen);
					//Sleep(1000);

				}else if (strcmp(recvbuf, "Move Left") == 0)
				{
					//std::string poseString = (collector.currentPose.toString());

					//String^ poseStrorageString = gcnew String(poseString.c_str());
					if (SerialPortToDevice->IsOpen) {
						for (int i = 0; i < 3; i++){
							SerialPortToDevice->WriteLine("waveIn");
							Sleep(300);
						}
					}
					ZeroMemory(recvbuf, recvbuflen);
					//Sleep(1000);
				}else if (strcmp(recvbuf, "Move Up") == 0)
				{
					//std::string poseString = (collector.currentPose.toString());

					//String^ poseStrorageString = gcnew String(poseString.c_str());
					if (SerialPortToDevice->IsOpen) {
						for (int i = 0; i < 3; i++){
							SerialPortToDevice->WriteLine("fingersSpread");
							Sleep(300);
						}
					}
					ZeroMemory(recvbuf, recvbuflen);
					//Sleep(1000);
				}else if (strcmp(recvbuf, "Move Down") == 0)
				{
					//std::string poseString = (collector.currentPose.toString());

					//String^ poseStrorageString = gcnew String(poseString.c_str());
					if (SerialPortToDevice->IsOpen) {
						for (int i = 0; i < 3; i++){
							SerialPortToDevice->WriteLine("fist");
							Sleep(300);
						}
					}
					ZeroMemory(recvbuf, recvbuflen);
					//Sleep(1000);
				}else if (strcmp(recvbuf, "Move Gripper") == 0)
				{
					//std::string poseString = (collector.currentPose.toString());

					//String^ poseStrorageString = gcnew String(poseString.c_str());
					if (SerialPortToDevice->IsOpen) {
							SerialPortToDevice->WriteLine("doubleTap");
							Sleep(300);

					}
					ZeroMemory(recvbuf, recvbuflen);
					//Sleep(1000);
				}
				else {
					std::string poseString = (collector.currentPose.toString());
					String^ poseStrorageString = gcnew String(poseString.c_str());
					printf("Only gesture: %s\n", poseStrorageString);
					if (SerialPortToDevice->IsOpen) {
						SerialPortToDevice->WriteLine(poseStrorageString);
						poseStrorageString = "";
					}
				}
				

			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << std::endl;
			std::cerr << "Press enter to continue.";
			std::cin.ignore();
			return 1;
		}
		//Sleep(1000);
	//}
	return 0;
}


int main()
{


	PMYDATA pDataArray[MAX_THREADS];
	DWORD   dwThreadIdArray[MAX_THREADS];
	HANDLE  hThreadArray[MAX_THREADS];
	int num = 100;

	pDataArray[0] = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(MYDATA));

	if (pDataArray[0] == NULL)
	{
		ExitProcess(2);
	}

	pDataArray[0]->val1 = &num;
	hThreadArray[0] = CreateThread(NULL, 0, MyThreadFunction1, pDataArray[0], 0, &dwThreadIdArray[0]);


	if (hThreadArray[0] == NULL)
	{
		ErrorHandler(TEXT("CreateThread"));
		ExitProcess(3);
	}

	pDataArray[1] = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(MYDATA));

	if (pDataArray[1] == NULL)
	{
		ExitProcess(2);
	}

	pDataArray[1]->val1 = &num;
	hThreadArray[1] = CreateThread(NULL, 0, MyThreadFunction2, pDataArray[1], 0, &dwThreadIdArray[1]);


	if (hThreadArray[1] == NULL)
	{
		ErrorHandler(TEXT("CreateThread"));
		ExitProcess(3);
	}

	WaitForMultipleObjects(MAX_THREADS, hThreadArray, TRUE, INFINITE);

	for (int i = 0; i<MAX_THREADS; i++)
	{
		CloseHandle(hThreadArray[i]);
		if (pDataArray[i] != NULL)
		{
			HeapFree(GetProcessHeap(), 0, pDataArray[i]);
			pDataArray[i] = NULL;    // Ensure address is not reused.
		}
	}

	return 0;

}

void ErrorHandler(LPTSTR lpszFunction)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

