#define _WINSOCK_DEPRECATED_NO_WARNINGS


//#ifndef UNICODE
//#define UNICODE
//#endif

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include<iostream> 
#include<time.h> 

#pragma comment(lib, "Ws2_32.lib")

struct UserEntity 
{
	int ThreadExitFlag;
	char* Nickname;
	SOCKET LinkedSocket;
	UserEntity* NextUserAddress;
};

struct MessageStruct
{
	char* Message;
	MessageStruct* NextMessageAddress;
};

void UnpatchIPSRC(char data[])
{
	printf("  %hhu.%hhu.%hhu.%hhu", data[12], data[13], data[14], data[15]);
}
void TryGetHostName(char address[])
{
	struct hostent* remoteHost;
	remoteHost = gethostbyaddr(&address[12], 4, AF_INET);
	if (remoteHost != NULL)
	{
		printf("   (%s)\n", remoteHost->h_name);
	}
	else
	{
		printf("\n");
	}
}

void ListenUser(UserEntity* User, MessageStruct* HeadMessage)
{
	char Data[1001];
	Data[1000] = 0;
	int ResivedSize;
	MessageStruct* TempAddress;
	MessageStruct* NewMessage;

	SOCKET ConnectedSocket = User->LinkedSocket;

	while ((ResivedSize = recv(ConnectedSocket, &Data[0], 1000, 0)) != SOCKET_ERROR)
	{
		Data[ResivedSize] = 0;

		if ((strcmp("givehistory", Data)) == 0) 
		{
			TempAddress = HeadMessage->NextMessageAddress;
			while (TempAddress != NULL)
			{
				send(ConnectedSocket,TempAddress->Message,strlen(TempAddress->Message),0);
				Sleep(350);
				TempAddress = TempAddress->NextMessageAddress;
			}
			send(ConnectedSocket,"end",3,0);
			continue;
		}

		printf("%s\n", Data);

		TempAddress = HeadMessage;
		while (TempAddress->NextMessageAddress != NULL)
		{
			TempAddress = TempAddress->NextMessageAddress;
		}
		NewMessage = new MessageStruct;
		NewMessage->Message = new char[1001];
		NewMessage->Message[0] = 0;
		strcpy_s(NewMessage->Message,1000, Data);
		NewMessage->NextMessageAddress = NULL;
		TempAddress->NextMessageAddress = NewMessage;

	}

	closesocket(User->LinkedSocket);
	User->LinkedSocket = NULL;
	return;
}


void TryFindChat(char* BroadcastAddress, int PortForBroadcast, char* Nickname)
{
	char* Data;
	sockaddr_in PacketDST;

	PacketDST.sin_family = AF_INET;
	PacketDST.sin_port = htons(PortForBroadcast);

	PacketDST.sin_addr.s_addr = inet_addr(BroadcastAddress);

	SOCKET BroadcastSocket;

	BroadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (BroadcastSocket == INVALID_SOCKET)
	{
		printf("Socket error: \n");
		WSACleanup();
		return;
	}

	int NicknameLength = 0;
	NicknameLength = strlen(Nickname);

	Data = new char[NicknameLength+3];
	Data[0] = PortForBroadcast >>  8;
	Data[1] = PortForBroadcast % 256;

	for (int i = 0; i < NicknameLength; i++)
	{
		Data[i + 2] = Nickname[i];
	}
	Data[NicknameLength + 2] = 0;

	BOOL Broadcasting = TRUE;
	int n;

	n = setsockopt(BroadcastSocket, SOL_SOCKET, SO_BROADCAST, (char*)&Broadcasting, sizeof(BOOL));

	for (int i = 0; i < 1; i++)
	{
		n = sendto(BroadcastSocket, Data, NicknameLength + 3, 0, (sockaddr*)&PacketDST, sizeof(PacketDST));
	}
	closesocket(BroadcastSocket);
}

void ConnectToChat(int PortForListenSocket, UserEntity* Head, MessageStruct* HeadMessage)
{
	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (ListenSocket == INVALID_SOCKET)
	{
		printf("Socket error: \n");
		WSACleanup();
		return;
	}

	sockaddr_in ReciveAddress;
	ReciveAddress.sin_family = AF_INET;
	ReciveAddress.sin_port = htons(PortForListenSocket);
	ReciveAddress.sin_addr.s_addr = INADDR_ANY;

	if (bind(ListenSocket, (sockaddr*)&ReciveAddress, sizeof(ReciveAddress))) 
	{
		printf("Error bind\n");
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	if (listen(ListenSocket, 0x100))
	{
		printf("Error listen\n");
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}


	SOCKET UserSocket;
	sockaddr_in UserAddress;
	int UserAddressSize = sizeof(UserAddress);
	UserEntity* User;
	UserEntity* Temp = Head;
	char Nickname[101];
	int Flag = 0;

	char* Message;
	char Time[32];

	time_t Seconds;
	tm* TimeInfo = new tm;


	UserSocket = accept(ListenSocket, (sockaddr*)&UserAddress, &UserAddressSize);
	while (UserSocket != INVALID_SOCKET) 
	{	
		User = new UserEntity;
		User->LinkedSocket = UserSocket;
		User->ThreadExitFlag = 0;
		User->NextUserAddress = NULL;
		while (Temp->NextUserAddress != NULL)
		{
			Temp = Temp->NextUserAddress;
		}
		Temp->NextUserAddress = User;

		int NicknameLength = recv(UserSocket,Nickname,100,0);

		User->Nickname = new char[NicknameLength +1];
		for (int i = 0; i < NicknameLength; i++)
		{
			User->Nickname[i] = Nickname[i];
		}
		User->Nickname[NicknameLength] = 0;

		Seconds = time(NULL);
		localtime_s(TimeInfo, &Seconds);
		asctime_s(Time, 32, TimeInfo);
		Time[strlen(Time) - 1] = 0;

		Message = new char[1001];
		Message[0] = 0;
		strcat_s(Message, 1000, "[");
		strcat_s(Message, 1000, Time);
		strcat_s(Message, 1000, "] ");
		strcat_s(Message, 1000, "User ");
		strcat_s(Message, 1000, Head->Nickname);
		strcat_s(Message, 1000, " connected to chat");

		send(UserSocket, Message, strlen(Message), 0);

		if (Flag == 0) 
		{
			Flag = 1;
			//puts(Message);

			send(UserSocket,"givehistory",11,0);

			MessageStruct* TempMessage;
			MessageStruct* NewMessage;
			char* History;
			int RecivedBytes;
			History = new char[1001];

			while ( (RecivedBytes = recv(UserSocket, History, 1000,0)) != INVALID_SOCKET )
			{
				History[RecivedBytes] = 0;
				
				if ((strcmp("end", History)) == 0) 
				{
					break;
				}
				
				TempMessage = HeadMessage;
				while (TempMessage->NextMessageAddress != NULL)
				{
					TempMessage = TempMessage->NextMessageAddress;
				}
				NewMessage = new MessageStruct;
				NewMessage->Message = History;
				NewMessage->NextMessageAddress = NULL;
				TempMessage->NextMessageAddress = NewMessage;

				printf("%s\n", History);
				History = new char[1001];
			}

		}

		std::thread ListenUser(ListenUser, User, HeadMessage);
		ListenUser.detach();

		Message = NULL;
		Temp = Head;

		UserSocket = accept(ListenSocket, (sockaddr*)&UserAddress, &UserAddressSize);
	}
	closesocket(ListenSocket);
}

void ConnetToNewUser(int PortForBroadcast, int PortForListenSocket, UserEntity* Head, MessageStruct* HeadMessage)
{
	char Data[1024];

	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("Socket error: \n");
		WSACleanup();
		return;
	}

	BOOL Broadcasting = TRUE;
	int n;

	n = setsockopt(ListenSocket, SOL_SOCKET, SO_BROADCAST, (char*)&Broadcasting, sizeof(BOOL));

	sockaddr_in ReciveAddress;
	ReciveAddress.sin_family = AF_INET;
	ReciveAddress.sin_port = htons(PortForBroadcast);
	ReciveAddress.sin_addr.s_addr = INADDR_ANY;

	if (bind(ListenSocket, (sockaddr*)&ReciveAddress, sizeof(ReciveAddress)))
	{
		printf("Error bind\n");
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}


	sockaddr_in BroadcastSRC;
	int Size = sizeof(sockaddr_in);
	sockaddr_in ConnectAddress;	
	UserEntity* User;
	UserEntity* Temp = Head;

	SOCKET ConnectToUserSocket;

	int RecivedSize;

	while ( (RecivedSize = recvfrom(ListenSocket, Data, 1024, 0, (sockaddr*)&BroadcastSRC, &Size)) != INVALID_SOCKET )
	{

		ConnectToUserSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (ConnectToUserSocket == INVALID_SOCKET)
		{
			printf("Socket error: \n");
			WSACleanup();
			return;
		}

		ConnectAddress.sin_addr.S_un.S_addr = BroadcastSRC.sin_addr.S_un.S_addr;
		ConnectAddress.sin_family = BroadcastSRC.sin_family;
		ConnectAddress.sin_port = htons(PortForListenSocket);
		
		if (connect(ConnectToUserSocket, (sockaddr*)&ConnectAddress, sizeof(ConnectAddress)))
		{
			printf("Connect error %d\n",WSAGetLastError());
			closesocket(ConnectToUserSocket);
			return;
		}

		int n = send(ConnectToUserSocket,Head->Nickname,strlen(Head->Nickname),0);

 		User = new UserEntity;
		User->LinkedSocket = ConnectToUserSocket;
		User->ThreadExitFlag = 0;
		User->Nickname = NULL;
		User->NextUserAddress = NULL;
		while (Temp->NextUserAddress != NULL)
		{
			Temp = Temp->NextUserAddress;
		}
		Temp->NextUserAddress = User;
			
		User->Nickname = new char[RecivedSize - 1];

		for (int i = 2; i < RecivedSize; i++)
		{
			User->Nickname[i - 2] = Data[i];
		}
		User->Nickname[RecivedSize - 2] = 0;	

		std::thread ListenUser(ListenUser, User, HeadMessage);

		ListenUser.detach();
	
		Temp = Head;

	}

	closesocket(ListenSocket);

}



int main()
{
	char TempInfo[1024];

	if (WSAStartup(0x0202, (WSADATA*)&TempInfo[0]))
	{
		printf("WSAStartup error \n");
		return -1;
	}

	char* IP = new char[20];
	char TempBufer[201];
	gethostname(TempBufer, 200);
	if (LPHOSTENT Localhost = gethostbyname(TempBufer)) 
	{
		strcpy_s(IP,20, inet_ntoa(*((in_addr*)Localhost->h_addr_list[0])));
	}

	char BroadcastAddress[] = "255.255.255.255";

	char* Nickname = new char[101];
	printf("Choose nickname: ");
	gets_s(Nickname, 100);

	TryFindChat(BroadcastAddress, 80, Nickname);

	UserEntity* Head;
	Head = new UserEntity;
	Head->LinkedSocket = NULL;
	Head->Nickname = new char[strlen(Nickname)+1];
	for (int i = 0; i < strlen(Nickname); i++)
	{
		Head->Nickname[i] = Nickname[i];
	}
	Head->Nickname[strlen(Nickname)] = 0;
	Head->ThreadExitFlag = 0;
	Head->NextUserAddress = NULL;

	MessageStruct* MessageHead;
	MessageHead = new MessageStruct;
	MessageHead->Message = NULL;
	MessageHead->NextMessageAddress = NULL;
	MessageStruct* TempAddress;
	MessageStruct* NewMessage;

	int CloseThreadConnetToNewUserThread = 0;

	std::thread ConnectToChatThread(ConnectToChat, 100, Head, MessageHead);
	std::thread ConnetToNewUserThread(ConnetToNewUser, 80, 100, Head, MessageHead);
	
	ConnectToChatThread.detach();
	ConnetToNewUserThread.detach();

	Sleep(1000);

	char Data[5119];
	UserEntity* User;
	SOCKET SendSockets;

	time_t Seconds;
	tm* TimeInfo = new tm;
	char Time[32];
	char* Message;

	while (true)
	{
		gets_s(Data, 300);
		
		if (strcmp("", Data) == 0) { continue; }

		if (strcmp("quit", Data) == 0) 
		{
			Message = new char[1001];
			Message[0] = 0;

			Seconds = time(NULL);
			localtime_s(TimeInfo, &Seconds);
			asctime_s(Time, 32, TimeInfo);
			Time[strlen(Time) - 1] = 0;

			strcat_s(Message, 1000, "[");
			strcat_s(Message, 1000, Time);
			strcat_s(Message, 1000, "] ");
			strcat_s(Message, 1000, "User ");
			strcat_s(Message, 1000, Head->Nickname);
			strcat_s(Message, 1000, " left the chat");

			User = Head->NextUserAddress;
			while (User != NULL)
			{
				SendSockets = User->LinkedSocket;
				send(SendSockets, Message, strlen(Message), 0);
				User = User->NextUserAddress;
			}
			
			Sleep(1000);

			User = Head->NextUserAddress;
			while (User != NULL)
			{
				SendSockets = User->LinkedSocket;
				closesocket(SendSockets);
				User->LinkedSocket = NULL;
				User = User->NextUserAddress;
			}

			CloseThreadConnetToNewUserThread = 1;

			break;
		}

		if ((strcmp("history", Data)) == 0) 
		{
			puts("------------------------------------------");
			TempAddress = MessageHead->NextMessageAddress;
			while (TempAddress != NULL)
			{
				puts(TempAddress->Message);
				TempAddress = TempAddress->NextMessageAddress;
			}

			continue;
		}

		Seconds = time(NULL);
		localtime_s(TimeInfo,&Seconds);
		asctime_s(Time,32,TimeInfo);
		Time[strlen(Time)-1]=0;


		Message = new char[1001];
		Message[0] = 0;
		strcat_s(Message, 1000 ,"[");
		strcat_s(Message, 1000, Time);
		strcat_s(Message, 1000,"] ");
		strcat_s(Message, 1000, Head->Nickname);
		strcat_s(Message, 1000, "(");
		strcat_s(Message, 1000, IP);
		strcat_s(Message, 1000, ") >>> ");
		strcat_s(Message, 1000, Data);

		puts(Message);

		TempAddress = MessageHead;
		while (TempAddress->NextMessageAddress != NULL)
		{
			TempAddress = TempAddress->NextMessageAddress;
		}
		NewMessage = new MessageStruct;
		NewMessage->Message = Message;
		NewMessage->NextMessageAddress = NULL;
		TempAddress->NextMessageAddress = NewMessage;

		User = Head->NextUserAddress;
		while (User != NULL)
		{
			SendSockets = User->LinkedSocket;
			send(SendSockets, Message,strlen(Message),0);
			User = User->NextUserAddress;
		}

		Message = NULL;
	}

	WSACleanup();

	system("Pause");
}
