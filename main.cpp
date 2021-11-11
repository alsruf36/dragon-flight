/*
Dragon Flight by Mingyeol Kim, Sujung Lee

=== 게임 설명 ===
이 게임은 라인 게임즈의 드레곤 플라이트를 콘솔 버전으로 모작한 게임입니다.

[Dragons, 용]
 - 흰색 : 0m 이후에서 나옵니다. (50점)
 - 노란색 : 250m 이후에서 나옵니다. (100점)
 - 초록색 : 3000m 이후에서 나옵니다. (200점)
 - 빨간색 : 9800m 이후에서 나옵니다. (300점)
 - 보라색 : 17500m 이후에서 나옵니다. (17500점)
 - 폭탄(OO색) : 같은 줄의 모든 용을 제거합니다.

[Meteorite, 운석]
 빨간색 느낌표로 줄이 생성되며 운석이 떨어질 것을 예고합니다.
 운석은 매우 빠른 속력으로 내려오며 2X2 크기입니다.
 동시에 최대 2개의 운석까지 생성이 됩니다.

=== 클래스 설명 ===
class JSON
-> 사용자의 데이터를 저장/로드/관리 하기 위한 클래스입니다.
-> 사용자의 데이터를 json 이라는 데이터 구조를 이용하여 저장/로드합니다.

class Frame
-> 게임의 화면을 출력하기 위한 클래스입니다.
-> 매 프레임마다 패치 및 출력을 합니다.

class Game
-> 게임의 논리 구현부 입니다.
-> 매 클럭마다 플레이어의 위치를 연산하며, 배열을 출력합니다.
-> 매 프레임 마다 게임 구성 요소(몬스터, 운석, 플레이어와의 충돌 등)를 움직이는 연산을 합니다.

TODO
-> 몬스터 구현
-> 수행 제출 빌드 전 헤더 파일 분리
-> 체력 표시 : 몬스터의 밝기로 판단

-> 몬스터 체력 구현을 위하여 기존 배열을 구조체로 바꾸기
-> 위와 같이 구현할 시 
*/

//IO 컨트롤
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <utility>
#include <conio.h>
#include <Windows.h>
#include <tchar.h>
#include <fstream>
#include <sstream>
using namespace std;

//색깔 정의
#define BLUE 1 //어두움
#define GREEN 2
#define BLUEGREEN 3
#define RED 4
#define PURPLE 5
#define YELLOW 6
#define WHITE 7
#define GRAY 8
#define SKYBLUE 9
#define B_GREEN 10 //밝음
#define B_BLUEGREEN 11
#define B_RED 12
#define B_PURPLE 13
#define B_YELLOW 14
#define B_WHITE 15

//오브젝트 정의
#define NONE 0
#define PLAYER 1
#define BULLET 2
#define WHITE_DRAGON 3

//체력 정의
#define H_PLAYER 3
#define H_BULLET 1
#define H_WHITE_DRAGON 1

//점수 정의

typedef struct Element{
    int object; //자신의 오브젝트 번호
    int health = 0; //자신의 체력
} Element;

//콘솔창 제어 함수
namespace Console{
    HANDLE hStdin;
    DWORD fdwSaveOldMode;
    DWORD cNumRead, fdwMode, ii;
    INPUT_RECORD irInBuf;
    int counter = 0;

    typedef struct xy{
        int x;
        int y;
    } xy;

    void init(){
        system("chcp 65001");
    }

    void sleep(float sec){
        Sleep(sec * 1000.0);
    }

    void cls(){
        system("cls");
    }

    void windowSize(int x, int y){
        string query = "mode con cols=" + to_string(x) + " lines=" + to_string(y);
        system(&query[0]);
    }

    void cursorVisible(bool status){
        CONSOLE_CURSOR_INFO cursorInfo = { 0, };
        cursorInfo.dwSize = 100;
        if(status == true) cursorInfo.bVisible = TRUE; 
        else cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    }

    void gotoxy(int x, int y){
        COORD Pos;
        Pos.X = 2 * x;
        Pos.Y = y;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
    }

    void setColor(int text, int back){
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text | (back << 4));
    }

    void ErrorExit(LPCSTR lpszMessage){
        fprintf(stderr, "%s\n", lpszMessage);
        SetConsoleMode(hStdin, fdwSaveOldMode);
        ExitProcess(0);
    }

    void useMouse(bool status){
        if(status == true){
            hStdin = GetStdHandle(STD_INPUT_HANDLE);
            if (hStdin == INVALID_HANDLE_VALUE)
                ErrorExit("GetStdHandle");

            if (!GetConsoleMode(hStdin, &fdwSaveOldMode))
                ErrorExit("GetConsoleMode");

            fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS;
            if (!SetConsoleMode(hStdin, fdwMode))
                ErrorExit("SetConsoleMode");

        }else{
            SetConsoleMode(hStdin, fdwSaveOldMode);
        }
    }

    void getMousexy(xy *mousexy){
        if (!ReadConsoleInput(
            hStdin, &irInBuf, 1, &cNumRead))
            ErrorExit("ReadConsoleInput");
        
            if (irInBuf.EventType == MOUSE_EVENT){
                int mouse_x = irInBuf.Event.MouseEvent.dwMousePosition.X;
                int mouse_y = irInBuf.Event.MouseEvent.dwMousePosition.Y;
                mousexy->x = mouse_x;
                mousexy->y = mouse_y;
            }else{
                mousexy->x = -1;
                mousexy->y = -1;
            }
    }
}

//JSON 컨트롤
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
using namespace rapidjson;

class JSON{
    public:
        Document d;

        string DataFile; //파일 이름
        JSON(string Datafile); //생성자
        void load();
        void save();
};

JSON::JSON(string DataFile){
    this->DataFile = DataFile;
    this->load();
}

void JSON::load(){
    FILE* fp;
    fp = fopen(&this->DataFile[0], "rb+");

    if(fp == NULL){
        cout << this->DataFile << " 로드에 오류가 생겼습니다." << endl;
        return;
    }
 
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    this->d.ParseStream(is);
    
    fclose(fp);
}

void JSON::save(){
    FILE* fp;
    fp = fopen(&this->DataFile[0], "wb+");

    char writeBuffer[65536];
    FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    
    Writer<FileWriteStream> writer(os);
    this->d.Accept(writer);
    
    fclose(fp);
}

//=================================== 메인 코드 ===================================

class Frame{
    public:
        //frame 설정
        int fps; //초당 프레임
        int SkipFramePer; //출력할 프레임 배수(나머지 프레임은 출력을 하지 않음)
        int horizontal; //가로
        int vertical; //세로
        int **frame; //frame 포인터
        double interval; //fps에 따른 frame갱신 시간
        char *Dprefix;

        void printLogo(int x, int y); //로고 프린트
        int LogoVertical = 3; //로고 세로 길이

        void print();
        Frame(int fps, int horizontal, int vertical); //생성자
};

Frame::Frame(int fps, int horizontal, int vertical){
    this->fps = fps;
    this->horizontal = horizontal;
    this->vertical = vertical;
    this->interval = 1.0/(float)(this->fps);

    int **frameVertical = new int *[this->vertical];
    for(int i=0;i < this->vertical;i++){
        frameVertical[i] = new int [this->horizontal];
    }

    this->frame = frameVertical;
    this->SkipFramePer = 1;

    this->Dprefix = "█";
    Console::init();
}

/*
[Frame::print()]
다음의 알고리즘을 시행합니다.
*/
void Frame::print(){
    Console::gotoxy(0, 0);
    for(int v=0;v<this->horizontal + 2;v++) printf("%s", this->Dprefix);
    printf("\n");

    for(int v=0;v<this->vertical;v++){
        printf("%s", this->Dprefix);
        for(int h=0;h<this->horizontal;h++){
            if(this->frame[v][h] != 0)
                printf("%d", this->frame[v][h]);
            else printf(" ");
        }
        printf("%s\n", this->Dprefix);
    }

    for(int v=0;v<this->horizontal + 2;v++) printf("%s", this->Dprefix);
    printf("\n");
}

void Frame::printLogo(int x, int y){
    int nowline = 0;
    string line;
    fstream logo;
    logo.open("logo.txt", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy(x, y + nowline++);
        cout << line << endl;
    }
}

class Game{
    public:
        JSON *json; //JSON 클래스 포인터
        string DataFile; //json 파일명

        Frame *printframe; //Frame 클래스 포인터
        int **frame; //frame 포인터
        int t_clock; //현재 clock(0 ~ this->printframe->fps)
        int m_clock; //현재 clock(t_clock이 초기화 된 횟수)

        int getKEY(); //키 버퍼 감지 / this->printframe->interval에 따른 sleep
        int streamKEY(int key); //감지된 키 스트림(중계)

        int PlayerHorizontal; //플레이어의 가로 위치
        int FrameClock; //프레임을 갱신할 클럭
        int patchMonsterFrame; //몬스터를 패치할 프레임 배수 
        int bulletClock; //총알을 패치할 클럭 배수
        void makeClockFrame(); //연산 클럭을 생성함
        bool updateFrame(); //배열을 조작함
        void patchPlayer(Console::xy coor); //플레이어의 가로 위치를 프레임에 패치
        void patchMonster(); //프레임의 맨 윗줄에 몬스터를 패치
        bool shiftFrame(); //맨 윗줄부터 플레이어 이전 줄을 한 칸 아래로 밂
        void makeFrame(); //매 클럭당 출력
        void Over(); //몬스터와 플레이어 충돌시 실행되는 함수

        void init(); //새 게임 시작 전 초기화자
        Game(string DataFile); //생성자
};

Game::Game(string DataFile){
    this->DataFile = DataFile;
    this->json = new JSON(this->DataFile);
    this->printframe = new Frame(2000, 15, 30);
    this->frame = this->printframe->frame;

    this->FrameClock = 10;
    this->patchMonsterFrame = 10;
    this->bulletClock = 1;
}

void Game::init(){
    for(int v=0;v<this->printframe->vertical;v++){
        for(int h=0;h<this->printframe->horizontal;h++){
            this->frame[v][h] = 0;
        }
    }
    this->PlayerHorizontal = this->printframe->horizontal/2;
    this->frame[this->printframe->vertical-1][this->PlayerHorizontal] = 1;
    this->t_clock = 0;
    this->m_clock = 0;
    
    Console::windowSize(this->printframe->horizontal + 150, this->printframe->vertical + 10);
    Console::cls();
    Console::cursorVisible(false);
    this->printframe->printLogo(this->printframe->horizontal, 0);
    Console::useMouse(true);
}

int Game::getKEY(){
    int key = 0;
    while(1) {
        if(kbhit()) {
            key = getch();
            if(key == 224 || key == 0){
                key = getch();
                if(key == 75) this->streamKEY(2); //왼쪽
                else if(key == 77) this->streamKEY(1); //오른쪽
                else if(key == 72) this->streamKEY(3); //위
                else if (key == 80) this->streamKEY(4); //아래
            }
        }
        this->makeFrame();
    }
}

int Game::streamKEY(int key){ // 1==오른쪽, 2==왼쪽, 3==위, 4==아래
    printf("%d\n", key);
}

void waitMouse(promise<Console::xy> *p){
    Console::xy coor;
    Console::getMousexy(&coor);
    p->set_value(coor);
}

/*
[Game::makeClockFrame()]
다음의 알고리즘을 반복합니다.
1. 마우스의 움직임을 감지할 waitMouse() 스레드를 생성한다.

2. 만약 마우스의 움직임이 감지되어 좌표가 반환되면 (2-1) 아니면 (3)
2-1. patchPlayer() 함수를 이용하여 플레이어의 좌표를 패치한다.
2-2. 스레드를 다시 join시킨다.

3. updateFrame() 함수를 이용하여 특정 클럭 마다 프레임을 업데이트 한다.

4. makeFrame() 함수를 이용하여 매 클럭마다 프레임을 출력한다.

5. 만약) 스레드가 join 되었다면 1로, 아니라면 2로 간다.
*/
void Game::makeClockFrame(){
    bool gameStatus = true;
    while(1){
        promise<Console::xy> p;
        future<Console::xy> coor = p.get_future();
        thread t(waitMouse, &p);
        Console::xy xy;
        while (gameStatus) {
            future_status status = coor.wait_for(std::chrono::milliseconds((int)(this->printframe->interval * 1000)));
            if (status == future_status::timeout){
                gameStatus = this->updateFrame();
                this->makeFrame();
                if(gameStatus == false) break;
            }
            else if (status == future_status::ready){
                xy = coor.get();
                this->patchPlayer(xy);
                this->updateFrame();
                this->makeFrame();
                Console::sleep(this->printframe->interval);
                break;
            }
        }
        t.join();
    }
    this->Over();
}

/*
[Game::makeFrame()]
다음의 알고리즘을 시행합니다.
1. 만약 t_clock (작은 단위의 클럭)이 (fps-1)에 도달하면 1-1 아니면 2
1-1. m_clock (큰 단위의 클럭)을 1 증가시킨후 t_clock을 0으로 초기화한다.

2. t_clock을 1 증가시킨다.

3. 프레임을 출력한다.
*/
void Game::makeFrame(){
    if(this->t_clock == this->printframe->fps-1){
        this->m_clock++;
        this->t_clock = 0;
    }else this->t_clock++;
    if(this->t_clock % this->printframe->SkipFramePer == 0) this->printframe->print();
    Console::gotoxy(0, this->printframe->vertical+2);
    printf("%d %d          \n", this->t_clock, this->m_clock);
}

/*
[Game::updateFrame()]
다음의 알고리즘을 시행합니다.
*/
bool Game::updateFrame(){
    if(this->t_clock % this->bulletClock == 0) this->frame[this->printframe->vertical-2][this->PlayerHorizontal] = BULLET;
    if(this->shiftFrame() == false) return false;
    if(this->t_clock % this->FrameClock == 0) this->patchMonster();
    return true;
}

/*
[Game::patchPlayer()]
다음의 알고리즘을 시행합니다.
1. 만약 마우스의 x좌표가 출력되는 배열 내에 있다면 플레이어의 이전 죄표를 0으로 만들고 현재 좌표를 1도 만든다.
*/
void Game::patchPlayer(Console::xy coor){
    if(coor.x > 0 && coor.x < this->printframe->horizontal+1){
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal] = NONE;
        this->PlayerHorizontal = coor.x-1;
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal] = PLAYER;
    }
}

/*
[Game::patchMonster()]
다음의 알고리즘을 시행합니다.
*/
void Game::patchMonster(){
    if(this->t_clock % (this->FrameClock * this->patchMonsterFrame) == 0){
        for(int h=0;h<this->printframe->horizontal;h++){
            this->frame[0][h] = WHITE_DRAGON;
        }
    }
}

/*
[Game::shiftFrame()]
다음의 알고리즘을 시행합니다.
1. 만약 총알이 0 번째 줄에 존재한다면, 배열에서 제거한다. (BULLET -> NONE)
2. 1 번째 줄부터 (끝 - 1)줄 까지 총알이 존재한다면 한 칸씩 위로 옮겨준다.
*/
bool Game::shiftFrame(){
    for(int h=0;h<this->printframe->horizontal;h++){ //bullet remove (bullet가 첫 번째 줄에 도달했을 떄)
        if(this->frame[0][h] == 2) this->frame[0][h] = 0;
    }

    for(int v=1;v<this->printframe->vertical-1;v++){ //bullet shift
        for(int h=0;h<this->printframe->horizontal;h++){
            if(this->frame[v][h] == BULLET){
                this->frame[v-1][h] = BULLET;
                this->frame[v][h] = NONE;
            }
        }
    }

    if(this->t_clock % (this->FrameClock - this->m_clock) == 0){ //monster shift
        for(int v=this->printframe->vertical-2;v>=0;v--){
            for(int h=0;h<this->printframe->horizontal;h++){
                if(v == this->printframe->vertical-2){
                    if(this->frame[v][h] > BULLET && this->frame[v+1][h] == PLAYER){
                        //return false;
                        continue;
                    }
                    continue;
                }
                if(this->frame[v+1][h] == BULLET){
                    continue;
                }
                else if(this->frame[v][h] > BULLET && this->frame[v+1][h] == BULLET){
                    this->frame[v][h] = NONE;
                    this->frame[v+1][h] = NONE;
                }else{
                    this->frame[v+1][h] = this->frame[v][h];
                    this->frame[v][h] = NONE;
                }
            }
        }
    }
    return true;
}

void Game::Over(){
    Console::cursorVisible(true);
    Console::useMouse(false);
}

/*
[main()]
다음의 알고리즘을 시행합니다.
*/
int main(){
    Game game("data.json");
//    Value &v = game.json->d["user"];
//   cout << v.GetString() << endl;

    game.init();
    game.makeClockFrame();
}
