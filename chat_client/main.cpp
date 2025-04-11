#include <winsock2.h>
#include <iostream>
#include <string>
#include <WS2tcpip.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

void InitSocket()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "WSAStartup ����!" << endl;
        exit(1);
    }
}

SOCKET ConnectToServer()
{
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        cout << "���� ���� ����" << endl;
        exit(1);
    }

    SOCKADDR_IN serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    cout << "���� ���� �õ���..." << endl;
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cout << "���� ���� ����" << endl;
        exit(1);
    }

    cout << "���� ���� ����!" << endl;
    return clientSocket;
}

void ShowMenu()
{
    cout << "===== �ʱ� �޴� =====" << endl;
    cout << "1. ȸ������" << endl;
    cout << "2. �α���" << endl;
    cout << "3. ����" << endl;
}

void ShowChatMenu()
{
    cout << "===== ä�� �޴� =====" << endl;
    cout << "1. ä���ϱ�" << endl;
    cout << "2. �α׾ƿ�" << endl;
    cout << "3. ����" << endl;
}

string SendMsg(SOCKET clientSocket, string msg)
{
    send(clientSocket, msg.c_str(), msg.size(), 0);
    char buffer[1024] = { 0 };
    int recvSize = recv(clientSocket, buffer, (int)sizeof(buffer) - 1, 0);
    buffer[recvSize] = '\0';

    if (recvSize > 0)
    {
        cout << "���� ����: " << buffer << endl;
        return string(buffer);
    }
    return "";
}

int main()
{
    InitSocket();
    SOCKET clientSocket = ConnectToServer();

    while (true)
    {
        ShowMenu();
        int choice;
        cin >> choice;

        if (choice == 1)
        {
            string id, pw;
            cout << "���̵� �Է�: ";
            cin >> id;
            cout << "��й�ȣ �Է�: ";
            cin >> pw;
            string msg = "REGISTER:" + id + ":" + pw + ":";
            SendMsg(clientSocket, msg);
        }
        else if (choice == 2)
        {
            string id, pw;
            cout << "���̵� �Է�: ";
            cin >> id;
            cout << "��й�ȣ �Է�: ";
            cin >> pw;
            string msg = "LOGIN:" + id + ":" + pw + ":";
            string result = SendMsg(clientSocket, msg);

            if (result == "Login Success")
            {
                if (result == "Login Success")
                {
                    while (true) // ä�� �޴� �ݺ�
                    {
                        ShowChatMenu();
                        int subChoice;
                        cin >> subChoice;

                        if (subChoice == 1)
                        {
                            cin.ignore(); // ���� ����
                            while (true) // ä�� ���
                            {
                                string chat;
                                cout << "�޽��� �Է� (exit �Է� �� ä�� ����): ";
                                getline(cin, chat);

                                if (chat == "exit")
                                    break;  // ä�� ���� �� ä�� �޴��� ����

                                string chatMsg = "CHAT:" + chat;
                                SendMsg(clientSocket, chatMsg);
                            }
                        }
                        else if (subChoice == 2) // �α׾ƿ�
                        {
                            SendMsg(clientSocket, "exit");  // ������ �α׾ƿ� ó�� ��û
                            break;  // ä�� �޴� �� �ʱ� �޴� ����
                        }
                        else if (subChoice == 3) // ����
                        {
                            SendMsg(clientSocket, "exit"); // ���� ���� ���� ó��
                            closesocket(clientSocket);
                            WSACleanup();
                            exit(0); // ���α׷� ����
                        }
                    }
                }

                
            }
            
        }
        
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
