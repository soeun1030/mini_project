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
        cout << "WSAStartup 실패!" << endl;
        exit(1);
    }
}

SOCKET ConnectToServer()
{
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        cout << "소켓 생성 실패" << endl;
        exit(1);
    }

    SOCKADDR_IN serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    cout << "서버 연결 시도중..." << endl;
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cout << "서버 연결 실패" << endl;
        exit(1);
    }

    cout << "서버 연결 성공!" << endl;
    return clientSocket;
}

void ShowMenu()
{
    cout << "===== 초기 메뉴 =====" << endl;
    cout << "1. 회원가입" << endl;
    cout << "2. 로그인" << endl;
    cout << "3. 종료" << endl;
}

void ShowChatMenu()
{
    cout << "===== 채팅 메뉴 =====" << endl;
    cout << "1. 채팅하기" << endl;
    cout << "2. 로그아웃" << endl;
    cout << "3. 종료" << endl;
}

string SendMsg(SOCKET clientSocket, string msg)
{
    send(clientSocket, msg.c_str(), msg.size(), 0);
    char buffer[1024] = { 0 };
    int recvSize = recv(clientSocket, buffer, (int)sizeof(buffer) - 1, 0);
    buffer[recvSize] = '\0';

    if (recvSize > 0)
    {
        cout << "서버 응답: " << buffer << endl;
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
            cout << "아이디 입력: ";
            cin >> id;
            cout << "비밀번호 입력: ";
            cin >> pw;
            string msg = "REGISTER:" + id + ":" + pw + ":";
            SendMsg(clientSocket, msg);
        }
        else if (choice == 2)
        {
            string id, pw;
            cout << "아이디 입력: ";
            cin >> id;
            cout << "비밀번호 입력: ";
            cin >> pw;
            string msg = "LOGIN:" + id + ":" + pw + ":";
            string result = SendMsg(clientSocket, msg);

            if (result == "Login Success")
            {
                if (result == "Login Success")
                {
                    while (true) // 채팅 메뉴 반복
                    {
                        ShowChatMenu();
                        int subChoice;
                        cin >> subChoice;

                        if (subChoice == 1)
                        {
                            cin.ignore(); // 버퍼 비우기
                            while (true) // 채팅 모드
                            {
                                string chat;
                                cout << "메시지 입력 (exit 입력 시 채팅 종료): ";
                                getline(cin, chat);

                                if (chat == "exit")
                                    break;  // 채팅 종료 → 채팅 메뉴로 복귀

                                string chatMsg = "CHAT:" + chat;
                                SendMsg(clientSocket, chatMsg);
                            }
                        }
                        else if (subChoice == 2) // 로그아웃
                        {
                            SendMsg(clientSocket, "exit");  // 서버에 로그아웃 처리 요청
                            break;  // 채팅 메뉴 → 초기 메뉴 복귀
                        }
                        else if (subChoice == 3) // 종료
                        {
                            SendMsg(clientSocket, "exit"); // 서버 연결 종료 처리
                            closesocket(clientSocket);
                            WSACleanup();
                            exit(0); // 프로그램 종료
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
