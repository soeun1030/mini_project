#include <winsock2.h>
#include <windows.h>
#include <mysql/jdbc.h>
#include <thread>
#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

sql::Driver* driver;
sql::Connection* conn;
SOCKET serverSocket;

void InitSocket()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "WSAStartup 실패!" << endl;
        exit(1);
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        cout << "소켓 생성 실패!" << endl;
        exit(1);
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    SOCKADDR_IN serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9001);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (::bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cout << "bind 실패!" << endl;
        exit(1);
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cout << "listen 실패!" << endl;
        exit(1);
    }

    cout << "서버 실행중..." << endl;
}

void InitDB()
{
    try {
        driver = get_driver_instance();
        conn = driver->connect("tcp://127.0.0.1:3306", "root", "1030");
        conn->setSchema("minichat_db");
        cout << "DB 연결 성공" << endl;
    }
    catch (sql::SQLException& e)
    {
        cout << "DB 연결 실패: " << e.what() << endl;
        exit(1);
    }
}

void HandleRegister(string msg, SOCKET clientSocket)
{
    vector<string> tokens;
    size_t pos = 0;
    while ((pos = msg.find(":")) != string::npos)
    {
        tokens.push_back(msg.substr(0, pos));
        msg.erase(0, pos + 1);
    }

    string username = tokens[1];
    string password = tokens[2];

    sql::PreparedStatement* pstmt;
    sql::ResultSet* res;

    pstmt = conn->prepareStatement("SELECT * FROM users WHERE username=?");
    pstmt->setString(1, username);
    res = pstmt->executeQuery();

    string response;
    if (res->next())
        response = "ID already exists";
    else
    {
        pstmt = conn->prepareStatement("INSERT INTO users(username, password) VALUES(?, ?)");
        pstmt->setString(1, username);
        pstmt->setString(2, password);
        pstmt->execute();
        response = "Register Success";
    }

    send(clientSocket, response.c_str(), (int)response.size(), 0);
    delete res;
    delete pstmt;
}

bool HandleLogin(string msg, SOCKET clientSocket, int& user_id)
{
    vector<string> tokens;
    size_t pos = 0;
    while ((pos = msg.find(":")) != string::npos)
    {
        tokens.push_back(msg.substr(0, pos));
        msg.erase(0, pos + 1);
    }

    string username = tokens[1];
    string password = tokens[2];

    sql::PreparedStatement* pstmt;
    sql::ResultSet* res;

    pstmt = conn->prepareStatement("SELECT * FROM users WHERE username=? AND password=?");
    pstmt->setString(1, username);
    pstmt->setString(2, password);
    res = pstmt->executeQuery();

    string response;
    if (res->next())
    {
        user_id = res->getInt("user_id");
        pstmt = conn->prepareStatement("INSERT INTO user_sessions(user_id, ip_address) VALUES(?, ?)");
        pstmt->setInt(1, user_id);
        pstmt->setString(2, "127.0.0.1");
        pstmt->execute();
        response = "Login Success";
    }
    else
        response = "Login Failed";

    send(clientSocket, response.c_str(), (int)response.size(), 0);
    delete res;
    delete pstmt;
    return (response == "Login Success");
}

void HandleChat(string msg, SOCKET clientSocket, int user_id)
{
    string content = msg.substr(5); // CHAT: 이후

    sql::PreparedStatement* pstmt;
    pstmt = conn->prepareStatement("INSERT INTO message_log(sender_id, content) VALUES(?, ?)");
    pstmt->setInt(1, user_id);
    pstmt->setString(2, content);
    pstmt->execute();

    send(clientSocket, content.c_str(), (int)content.size(), 0);
    delete pstmt;
}

void HandleLogout(int user_id)
{
    try
    {
        if (conn && !conn->isClosed())
        {
            sql::PreparedStatement* pstmt;
            pstmt = conn->prepareStatement(
                "UPDATE user_sessions SET logout_time=NOW() WHERE user_id=? AND logout_time IS NULL");
            pstmt->setInt(1, user_id);
            pstmt->execute();
            delete pstmt;
        }
    }
    catch (sql::SQLException& e)
    {
        cout << "HandleLogout 에러 발생: " << e.what() << endl;
    }
}

void HandleClient(SOCKET clientSocket)
{
    char buffer[1024] = { 0 };
    int user_id = -1;

    while (true)
    {
        int recvSize = recv(clientSocket, buffer, (int)sizeof(buffer), 0);
        if (recvSize <= 0)
        {
            if (user_id != -1)
                HandleLogout(user_id);
            closesocket(clientSocket);
            break;
        }

        string msg(buffer);

        if (msg.find("REGISTER:") == 0)
            HandleRegister(msg, clientSocket);
        else if (msg.find("LOGIN:") == 0)
        {
            if (!HandleLogin(msg, clientSocket, user_id))
                user_id = -1;
        }
        else if (msg.find("CHAT:") == 0)
        {
            if (user_id != -1)
                HandleChat(msg, clientSocket, user_id);
        }
        else if (msg == "exit")
        {
            if (user_id != -1)
                HandleLogout(user_id);
            closesocket(clientSocket);
            break;
        }
    }
}

int main()
{
    InitDB();
    InitSocket();

    while (true)
    {
        SOCKET clientSocket;
        SOCKADDR_IN clientAddr = {};
        int addrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &addrSize);

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        cout << "접속한 클라이언트 IP: " << clientIP << endl;

        if (strcmp(clientIP, "127.0.0.1") == 0)
        {
            cout << "정상 클라이언트 접속 감지" << endl;
            thread clientThread(HandleClient, clientSocket);
            clientThread.detach();
        }
        else
        {
            cout << "비정상 접속 시도 차단!" << endl;
            closesocket(clientSocket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();

    if (conn && !conn->isClosed())
    {
        conn->close();
        delete conn;
    }
}
