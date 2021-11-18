/*
Dragon Flight by Mingyeol Kim, Sujung Lee

===! 주의 !===
이 파일은 UTF-8로 인코딩 되어있어 DEV-C++로 열 수 없습니다.
컴파일 시에는 C++11 이상을 사용하여야 합니다.
다음의 옵션으로 컴파일 하는것을 추천합니다.

    "command": "g++",
    "args": [
        "-static-libgcc",
        "-static-libstdc++",
        "-Wl,-Bstatic",
        "-lstdc++",
        "-lpthread",
        "-Wl,-Bstatic,--whole-archive",
        "-lwinpthread",
        "-Wl,-Bdynamic",
        "-Wl,--no-whole-archive",
        "-g",
        "${file}",
        "-o",
        "${fileDirname}/${fileBasenameNoExtension}.exe"
    ]

=== 게임 설명 ===
이 게임은 라인 게임즈의 드레곤 플라이트를 콘솔 버전으로 모작한 게임입니다.

[Dragons, 용]
 - 흰색 : 0m 이후에서 나옵니다. (50점)
 - 노란색 : 250m 이후에서 나옵니다. (100점)
 - 초록색 : 3000m 이후에서 나옵니다. (200점)
 - 빨간색 : 9800m 이후에서 나옵니다. (300점)
 - 보라색 : 17500m 이후에서 나옵니다. (500점)
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
-> 몬스터 구현 -> 완료
-> 수행 제출 빌드 전 헤더 파일 분리
-> 체력 표시 : 몬스터의 밝기로 판단

-> 몬스터 체력 구현을 위하여 기존 배열을 구조체로 바꾸기 -> 완료
-> 위와 같이 구현할 시 

-> 몬스터에 따라 점수 추가하기
-> 15 by 5 배열로 구성 or 출력되는 몬스터의 좌측 좌표를 기준으로 출력
-> 게임 오버 구현

-> 메인 화면 만들기
*/

//IO 컨트롤
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <Windows.h>
#include <tchar.h>
using namespace std;

//색깔 정의
#define BLACK 0 //어두움
#define BLUE 1
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
#define YELLOW_DRAGON 4
#define GREEN_DRAGON 5
#define RED_DRAGON 6
#define PURPLE_DRAGON 7

//체력 정의
#define H_NONE 0
#define H_PLAYER 3
#define H_BULLET 3
#define H_WHITE_DRAGON 1
#define H_YELLOW_DRAGON 2
#define H_GREEN_DRAGON 3
#define H_RED_DRAGON 4
#define H_PURPLE_DRAGON 5

//점수 정의
#define S_WHITE_DRAGON 50
#define S_YELLOW_DRAGON 150
#define S_GREEN_DRAGON 200
#define S_RED_DRAGON 300
#define S_PURPLE_DRAGON 500

//이벤트 정의
#define E_KEY_EVENT 1
#define E_MOUSE_EVENT 2
#define PAUSE_KEY 119
#define E_MOUSE_LEFT 1
#define E_MOUSE_RIGHT 2

typedef struct Element{
    int object; //자신의 오브젝트 번호
    int health = H_NONE; //자신의 체력
    Element* back; //오브젝트의 뒤에 중첩된 오브젝트
} Element;

//콘솔창 제어 함수
namespace Console{
    HANDLE hStdin;
    DWORD fdwSaveOldMode;
    DWORD cNumRead, fdwMode, i;
    INPUT_RECORD irInBuf;
    int counter = 0;

    typedef struct xy{
        int x;
        int y;
    } xy;

    typedef struct eventStruct{
        int eventType;
        int key;
        bool keyPressed;
        bool Clicked;
        bool ClickKey;
        xy coordinate;
    } eventStruct;

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
        system("pause > nul");
        ExitProcess(0);
    }

    void useEventInput(bool status){
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
    
    void getEvent(eventStruct* event){
        if (!ReadConsoleInput(hStdin, &irInBuf, 1, &cNumRead))
            ErrorExit("ReadConsoleInput");
        
        switch (irInBuf.EventType){
            case KEY_EVENT: {// keyboard 인풋일때
                char keyStr[5];
                sprintf(keyStr, "%d", irInBuf.Event.KeyEvent.uChar);
                event->key = atoi(keyStr);
                event->keyPressed = irInBuf.Event.KeyEvent.bKeyDown;
                event->eventType = E_KEY_EVENT;
                break;
            }

            case MOUSE_EVENT: {// mouse 인풋일때
                if(irInBuf.Event.MouseEvent.dwEventFlags == 0){
                    if (irInBuf.Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED){
                        event->Clicked = true;
                        event->ClickKey = E_MOUSE_LEFT;
                    }
                    else if (irInBuf.Event.MouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED){
                        event->Clicked = true;
                        event->ClickKey = E_MOUSE_RIGHT;
                    }
                }

                int mouse_x = irInBuf.Event.MouseEvent.dwMousePosition.X;
                int mouse_y = irInBuf.Event.MouseEvent.dwMousePosition.Y;
                event->eventType = E_MOUSE_EVENT;
                event->coordinate.x = mouse_x;
                event->coordinate.y = mouse_y;
                break;
            }
        }
    }

    void waitEvent(promise<eventStruct> *p){
        eventStruct event;
        event.eventType = NONE;
        while(event.eventType == NONE){
            getEvent(&event);
        }
        p->set_value(event);
    }
}

//=================================== 메인 코드 ===================================

class Frame{
    public:
        //frame 설정
        int fps; //초당 프레임
        int SkipFramePer; //출력할 프레임 배수(나머지 프레임은 출력을 하지 않음)
        int horizontal; //가로
        int vertical; //세로
        Element **frame; //frame 포인터
        double interval; //fps에 따른 frame갱신 시간

        int consolehorizontal; //콘솔창 가로
        int consolevertical; //콘솔창 세로

        void printMain(); //메인 화면 프린트
        void printPause(); //일시정지 화면 프린트
        void printLogo(); //로고 프린트
        void printScore(int score, int distance, int level, int levelCriteria, int PlayerHealth); //점수 프린트
        void printScoreframe(); //점수 프레임 프린트
        void printGameOver(); //게임 오버 화면 프린트
        int LogoVertical = 3; //로고 세로 길이
        int LeftSpace = 6; //게임 배열 좌측 공간
        int ScoreboardHeight = 3; //점수 프레임을 프린트할 때 위의 공간

        void print();
        Frame(int fps, int horizontal, int vertical); //생성자
};

Frame::Frame(int fps, int horizontal, int vertical){
    this->fps = fps;
    this->horizontal = horizontal;
    this->vertical = vertical;
    this->interval = 1.0/(float)(this->fps);

    this->frame = new Element *[this->vertical];
    for(int i=0;i < this->vertical;i++){
        this->frame[i] = new Element [this->horizontal];
    }

    this->SkipFramePer = 2;
    Console::init();
}

/*
[Frame::print()]
다음의 알고리즘을 시행합니다.
*/
void Frame::print(){
    Console::gotoxy(0, 0);
    Console::setColor(B_WHITE, BLACK);

    Console::gotoxy(this->LeftSpace/2, 0);
    printf("┌");
    for(int v=0;v<this->horizontal;v++) printf("─");
    printf("┐\n");

    int v;
    for(v=0;v<this->vertical;v++){
        Console::setColor(B_WHITE, BLACK);
        Console::gotoxy(this->LeftSpace/2, v+1);
        printf("│");
        for(int h=0;h<this->horizontal;h++){
            if(this->frame[v][h].object == NONE){
                Console::setColor(B_WHITE, BLACK);
                printf(" ");
            }
            else if(this->frame[v][h].object == BULLET){
                Console::setColor(B_WHITE, GRAY);
                printf("%d", this->frame[v][h].health);
            }
            else if(this->frame[v][h].object < BULLET){
                Console::setColor(B_WHITE, BLACK);
                printf("%d", this->frame[v][h].object);
            }
            else if(this->frame[v][h].object > BULLET){
                if(this->frame[v][h].object == WHITE_DRAGON) Console::setColor(BLACK, B_WHITE);
                else if(this->frame[v][h].object == YELLOW_DRAGON) Console::setColor(BLACK, B_YELLOW);
                else if(this->frame[v][h].object == GREEN_DRAGON) Console::setColor(BLACK, B_GREEN);
                else if(this->frame[v][h].object == RED_DRAGON) Console::setColor(BLACK, B_RED);
                else if(this->frame[v][h].object == PURPLE_DRAGON) Console::setColor(BLACK, B_PURPLE);
                else Console::setColor(B_WHITE, BLACK);
                printf("%d", this->frame[v][h].health);
            }
        }
        Console::setColor(B_WHITE, BLACK);
        printf("│\n");
    }

    Console::gotoxy(this->LeftSpace/2, v+1);
    printf("└");
    for(int v=0;v<this->horizontal;v++) printf("─");
    printf("┘\n");
}

void Frame::printLogo(){
    int nowline = 0;
    string line;
    fstream logo;
    logo.open("MAINLOGO", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy(this->consolehorizontal - 150, nowline++);
        cout << line << endl;
    }
}

void Frame::printScoreframe(){
    Console::gotoxy(this->horizontal, this->ScoreboardHeight);
    printf("┌");
    for(int h=0;h<22;h++) printf("─");
    printf("┐");

    int v;
    for(v=0;v<7;v++){
        Console::gotoxy(this->horizontal, this->ScoreboardHeight + 1 + v);
        printf("│");
        for(int h=0;h<22;h++) printf(" ");
        printf("│");
    }

    Console::gotoxy(this->horizontal, this->ScoreboardHeight + 1 + v);
    printf("└");
    for(int v=0;v<22;v++) printf("─");
    printf("┘\n");
}


void Frame::printScore(int score, int distance, int level, int levelCriteria, int PlayerHealth){
    Console::gotoxy(this->horizontal + 1, this->ScoreboardHeight + 2);
    printf("점수 : %d점", score);
    
    Console::gotoxy(this->horizontal + 1, this->ScoreboardHeight + 3);
    printf("거리 : %dm", distance);

    Console::gotoxy(this->horizontal + 1, this->ScoreboardHeight + 4);
    printf("체력 : ");
    Console::setColor(B_RED, BLACK);
    for(int i=0;i<PlayerHealth;i++) printf("H ");
    for(int i=0;i<H_PLAYER - PlayerHealth;i++) printf("  ");
    Console::setColor(B_WHITE, BLACK);

    Console::gotoxy(this->horizontal + 1, this->ScoreboardHeight + 5);
    printf("페이즈 : %d번째 ", level + 1);

    Console::gotoxy(this->horizontal + 1, this->ScoreboardHeight + 6);
    printf("현재 페이즈 [%.1lf%] ", ((double)(distance % levelCriteria)/(double)levelCriteria)*(double)100);
}

void Frame::printMain(){
    Console::gotoxy(0, 0);
    printf("┌");
    for(int i=0;i<this->consolehorizontal - 3;i++) printf("─");
    printf("┐");

    for(int i=0;i<this->consolevertical - 2;i++){
        Console::gotoxy(0, i+1);
        printf("│");
        for(int j=0;j<this->consolehorizontal - 3;j++) printf(" ");
        printf("│");
    }

    Console::gotoxy(0, this->consolevertical - 1);
    printf("└");
    for(int i=0;i<this->consolehorizontal - 3;i++) printf("─");
    printf("┘");

    int nowline = 0;
    string line;
    fstream logo;
    logo.open("MAINLOGO", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy((this->consolehorizontal - 100) / 4, 4 + nowline++);
        cout << line << endl;
    }

    Console::gotoxy((this->consolehorizontal - 14) / 4, 15);
    printf("시작하기 [W]");

    Console::gotoxy((this->consolehorizontal - 14) / 4, 19);
    printf("종료하기 [Q]");

    Console::gotoxy((this->consolehorizontal - 14) / 4, 23);
    printf("튜토리얼 [R]");
}

void Frame::printPause(){
    Console::gotoxy(0, 0);
    printf("┌");
    for(int i=0;i<this->consolehorizontal - 3;i++) printf("─");
    printf("┐");

    for(int i=0;i<this->consolevertical - 2;i++){
        Console::gotoxy(0, i+1);
        printf("│");
        for(int j=0;j<this->consolehorizontal - 3;j++) printf(" ");
        printf("│");
    }

    Console::gotoxy(0, this->consolevertical - 1);
    printf("└");
    for(int i=0;i<this->consolehorizontal - 3;i++) printf("─");
    printf("┘");

    int nowline = 0;
    string line;
    fstream logo;
    logo.open("PAUSELOGO", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy((this->consolehorizontal - 42) / 4, 4 + nowline++);
        cout << line << endl;
    }

    Console::gotoxy((this->consolehorizontal - 14) / 4, 15);
    printf("종료하기 [Q]");

    Console::gotoxy((this->consolehorizontal - 14) / 4, 19);
    printf("복귀하기 [W]");

    Console::gotoxy((this->consolehorizontal - 14) / 4, 23);
    printf("다시시작 [R]");
}

void Frame::printGameOver(){
    Console::gotoxy(0, 0);
    printf("┌");
    for(int i=0;i<this->consolehorizontal - 3;i++) printf("─");
    printf("┐");

    for(int i=0;i<this->consolevertical - 2;i++){
        Console::gotoxy(0, i+1);
        printf("│");
        for(int j=0;j<this->consolehorizontal - 3;j++) printf(" ");
        printf("│");
    }

    Console::gotoxy(0, this->consolevertical - 1);
    printf("└");
    for(int i=0;i<this->consolehorizontal - 3;i++) printf("─");
    printf("┘");

    int nowline = 0;
    string line;
    fstream logo;
    logo.open("GAMEOVERLOGO", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy((this->consolehorizontal - 74) / 4, 4 + nowline++);
        cout << line << endl;
    }

    Console::gotoxy((this->consolehorizontal - 14) / 4, 15);
    printf("종료하기 [Q]");

    Console::gotoxy((this->consolehorizontal - 14) / 4, 19);
    printf("다시시작 [R]");
}

class Game{
    public:
        Frame *printframe; //Frame 클래스 포인터
        Element **frame; //frame 포인터
        int distance; //현재 거리
        int level; //현재 레벨(levelCriteria의 배수마다 1 증가)
        int levelCriteria; //한 레벨을 올리는 데의 기준

        int SCREENpause(); //게임을 일시정지할 때 실행되는 함수

        int PlayerHorizontal; //플레이어의 가로 위치
        int FrameClock; //프레임을 갱신할 클럭 배수
        int patchMonsterClock; //몬스터를 패치할 클럭 배수 
        int bulletClock; //총알을 패치할 클럭 배수
        void makeClock(); //연산 클럭을 생성함
        bool updateFrame(); //배열을 조작함
        void patchPlayer(Console::xy coor); //플레이어의 가로 위치를 프레임에 패치
        void patchMonster(); //프레임의 맨 윗줄에 몬스터를 패치
        bool shiftFrame(); //맨 윗줄부터 플레이어 이전 줄을 한 칸 아래로 민다.
        void printFrame(); //매 클럭당 출력
        void Over(); //몬스터와 플레이어 충돌시 실행되는 함수

        void addScore(int target); //몬스터에 따라 score변수에 점수 추가
        Element randomMonster(int from, int to); //from부터 to까지 범위에 있는 몬스터를 랜덤으로 뽑아 구조체를 반환
        int score; //플레이어의 점수
        int PlayerHealth; //플레이어의 체력

        void init(); //새 게임 시작 전 초기화자
        Game(); //생성자
};

Game::Game(){ //생성자 : 메인 함수에서 클래스를 선언할 때 선언하자마자 호출없이 바로 살행되는 함수
    this->printframe = new Frame(2000, 15, 15); //frame 배열을 프린트하고, 관리할 Frame 클래스를 printframe이라는 이름으로 선언
    this->frame = this->printframe->frame; //game의 frame과 printframe의 frame이 같은 배열을 가르키도록 주소를 복사

    this->levelCriteria = 500; //한 레벨을 올리는 데의 기준
    this->FrameClock = 10; //FrameClock의 배수 클럭마다 프레임이 갱신된다.
    this->patchMonsterClock = 120; //patchMonsterClock의 배수 클럭마다 몬스터가 맨 윗줄에 패치된다.
    this->bulletClock = 4; //bulletClock의 배수 클럭마다 플레이어 바로 윗줄에 bullet이 생성이 된다.

    this->printframe->consolevertical = this->printframe->vertical + 15; //콘솔창의 가로 길이
    this->printframe->consolehorizontal = this->printframe->horizontal + 170; //콘솔창의 세로 길이
}

void Game::init(){ //게임을 새로 시작할 때 마다 게임 상황을 초기화해주는 함수
    for(int v=0;v<this->printframe->vertical;v++){ 
        for(int h=0;h<this->printframe->horizontal;h++){
            this->frame[v][h].object = NONE; //frame 배열의 모든 원소를 NONE으로 초기화한다.
            this->frame[v][h].health = H_NONE;
            this->frame[v][h].back = new Element;
            this->frame[v][h].back->object = NONE;
            this->frame[v][h].back->health = H_NONE;
        }
    }
    this->PlayerHealth = H_PLAYER;
    this->PlayerHorizontal = this->printframe->horizontal/2; //플레이어의 초기 좌표를 설정한다. [맨 밑줄 (가로길이/2)번째 칸을 지정]
    this->frame[this->printframe->vertical-1][this->PlayerHorizontal].object = PLAYER; //PlayerHorizontal 칸을 PLAYER로 지정한다.
    this->frame[this->printframe->vertical-1][this->PlayerHorizontal].health = this->PlayerHealth; //플레이어의 체력을 설정한다.
    this->distance = 0; //현재 거리
    this->level = 0; //현재 난이도
    this->score = 0; //점수
    
    srand(time(NULL)); //난수 시드 설정
    Console::windowSize(this->printframe->consolehorizontal, this->printframe->consolevertical); //윈도우 사이즈를 바꾼다.
    Console::cls(); //화면을 초기화
    Console::cursorVisible(false); //커서를 보이지 않게 한다.
    this->printframe->printLogo(); //지정된 위치에 로고를 프린트한다.
    this->printframe->printScoreframe(); //점수판 위치에 틀 프린트한다.
    Console::useEventInput(true); //마우스 사용을 선언한다.
}

/*
[Game::makeClock()]
다음의 알고리즘을 반복합니다.
1. 마우스의 움직임을 감지할 waitEvent() 스레드를 생성한다.

2. 만약 이벤트가 감지되어 좌표가 반환되면 (2-1) 아니면 (3)
2-1. patchPlayer() 함수를 이용하여 플레이어의 좌표를 패치한다.
2-2. 스레드를 다시 join시킨다.

3. updateFrame() 함수를 이용하여 특정 클럭 마다 프레임을 업데이트 한다.

4. printFrame() 함수를 이용하여 매 클럭마다 프레임을 출력한다.

5. 만약) 스레드가 join 되었다면 1로, 아니라면 2로 간다.
*/
void Game::makeClock(){
    bool gameStatus = true; //gameStatus을 true로 초기화
    while(1){ //게임이 종료될 때 까지 반복
        promise<Console::eventStruct> p; //p를 받겠다고 약속한다.
        future<Console::eventStruct> coor = p.get_future(); //coor을 통해 미래에 p를 받겠다고 선언한다.
        thread t(Console::waitEvent, &p); //waitEvent를 실행해 p에 받겠다는 약속을 하고 t라는 스레드를 생성한다.
        while (gameStatus) { //gameStatus가 false가 아니면 계속 반복한다.
            Console::eventStruct Event; //이벤트를 받을 구조체
            future_status status = coor.wait_for(std::chrono::milliseconds((int)(this->printframe->interval * 1000))); //미래에 받겠다고 한 coor이 완료가 되었는지 interval초 동안 물어본다.

             if(this->distance % this->levelCriteria == 0){ //만약 distance가 levelCriteria의 배수라면
                this->level++; //level을 1 증가시킨다.
            }
            this->distance++; //distance을 1 증가시킨다.
             
            if (status == future_status::timeout){ //만약 물어본지 1초가 지나 timeout되었다면(시간초과 되었다면)
                gameStatus = this->updateFrame(); //프레임을 업데이트한다.
                this->printFrame(); //프레임을 프린트한다.
                if(gameStatus == false) break; //만약 프레임을 업데이트 할 때 false가 반환이 되었으면 while문을 나간다.
            }
            else if (status == future_status::ready){ //만약 물어봤을때 함수의 반환이 준비가 되었다면
                Event = coor.get(); //미래에 받겠다고 한 정보를 반환받는다.

                if(Event.eventType == E_MOUSE_EVENT){
                    this->patchPlayer(Event.coordinate);
                }else if(Event.eventType == E_KEY_EVENT){
                    if(Event.keyPressed == true && Event.key == PAUSE_KEY){
                        this->SCREENpause();
                    }
                }

                this->updateFrame(); //프레임을 업데이트한다.
                this->printFrame(); //프레임을 출력한다.
                Console::sleep(this->printframe->interval); //interval초 동안 정지한다.
                break; //while문을 나간다.
            }
        }
        t.join(); //스레드 t를 종료한다.
    }
    this->Over(); //만약 계속 반복되던 while문이 break되어 종료되면 게임 오버로 간다.
}

/*
[Game::printFrame()]
다음의 알고리즘을 시행합니다.
1. 만약 클럭이 SkipFramePer의 배수라면 frame을 출력한다.
2. frame 밑에 distance과 levelCriteria을 출력한다.
*/
void Game::printFrame(){
    if(this->distance % this->printframe->SkipFramePer == 0){
        this->printframe->print();
        this->printframe->printScore(this->score, this->distance, this->level, this->levelCriteria, this->PlayerHealth);
    }
}

/*
[Game::updateFrame()]
다음의 알고리즘을 시행합니다.
1. 만약 클럭이 bulletClock의 배수라면 bullet을 플레이어 바로 윗 줄 같은 collum에 생성한다.
2. 만약 shiftFrame을 실행하다가 false가 반환되면 바로 updateFrame에서 false를 반환한다. (바로 게임 종료를 위함)
3. 만약 클럭이 FrameClock의 배수라면 몬스터를 패치한다.
4. true를 반환한다.
*/
bool Game::updateFrame(){
    if(this->distance % this->bulletClock == 0){
        this->frame[this->printframe->vertical-2][this->PlayerHorizontal].object = BULLET;
        this->frame[this->printframe->vertical-2][this->PlayerHorizontal].health = H_BULLET;
    }
    if(this->shiftFrame() == false) return false;
    if(this->distance % this->FrameClock == 0) this->patchMonster();
    return true;
}

/*
[Game::patchPlayer()]
다음의 알고리즘을 시행합니다.
1. 만약 마우스의 x좌표가 출력되는 배열 내에 있다면 플레이어의 이전 죄표를 0으로 만들고 현재 좌표를 1도 만든다.
*/
void Game::patchPlayer(Console::xy coor){
    if(coor.x > this->printframe->LeftSpace && coor.x < this->printframe->horizontal + 1 + this->printframe->LeftSpace){
        Console::gotoxy(0, this->printframe->vertical+5);
        printf("                                         ");
        for(int v=0;v<this->printframe->vertical + 2;v++){
            Console::gotoxy(this->printframe->LeftSpace/2 - 2, v);
            printf("  ");
            Console::gotoxy(this->printframe->LeftSpace/2 + this->printframe->horizontal/2 + 3, v);
            printf("  ");
        }
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal].object = NONE;
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal].health = H_NONE;
        this->PlayerHorizontal = coor.x - 1 - this->printframe->LeftSpace;
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal].object = PLAYER;
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal].health = this->PlayerHealth;
    }else{
        Console::gotoxy(0, this->printframe->vertical+5);
        Console::setColor(12, 0);
        printf("마우스를 플레이 범위 안으로 옮겨주세요!!");
        Console::setColor(15, 0);

        for(int v=0;v<this->printframe->vertical + 2;v++){
            Console::gotoxy(this->printframe->LeftSpace/2 - 2, v);
            printf("❯");
            Console::gotoxy(this->printframe->LeftSpace/2 + this->printframe->horizontal/2 + 3, v);
            printf("❮");
        }
    }
}

/*
[Game::randomMonster()]
다음의 알고리즘을 시행합니다.
1. 랜덤으로 생성된 값을 (to - from)으로 나눈 나머지를 구한다.
2. form에 [1]에서 구한 값을 더하면 반환할 몬스터의 번호가 나온다.
3. [2]에 맞도록 switch-case문을 써서 반환 값을 설정한다.
*/
Element Game::randomMonster(int from, int to){
    Element tmp;
    int target = from + rand() % (to - from + 1);

    switch(target){
        case WHITE_DRAGON:
            tmp.object = WHITE_DRAGON;
            tmp.health = H_WHITE_DRAGON;
            break;
        
        case YELLOW_DRAGON:
            tmp.object = YELLOW_DRAGON;
            tmp.health = H_YELLOW_DRAGON;
            break;

        case GREEN_DRAGON:
            tmp.object = GREEN_DRAGON;
            tmp.health = H_GREEN_DRAGON;
            break;

        case RED_DRAGON:
            tmp.object = RED_DRAGON;
            tmp.health = H_RED_DRAGON;
            break;

        case PURPLE_DRAGON:
            tmp.object = PURPLE_DRAGON;
            tmp.health = H_PURPLE_DRAGON;
            break;

        default:
            tmp.object = PURPLE_DRAGON;
            tmp.health = H_PURPLE_DRAGON;
            break;
    }

    return tmp;
}

/*
[Game::patchMonster()]
다음의 알고리즘을 시행합니다.
1. 만약 클럭이 patchMonsterClock의 배수이면 frame의 맨 윗줄에 몬스터를 패치한다.
*/
void Game::patchMonster(){
    if(this->distance % patchMonsterClock == 0){
        for(int h=0;h<this->printframe->horizontal;h++){
            Element tmp;

            switch(this->level){
                case 0:
                    tmp = this->randomMonster(WHITE_DRAGON, WHITE_DRAGON);
                    break;

                case 1:
                    tmp = this->randomMonster(WHITE_DRAGON, YELLOW_DRAGON);
                    break;
                
                case 2:
                    tmp = this->randomMonster(WHITE_DRAGON, GREEN_DRAGON);
                    break;

                case 3:
                    tmp = this->randomMonster(YELLOW_DRAGON, RED_DRAGON);
                    break;

                case 4:
                    tmp = this->randomMonster(GREEN_DRAGON, PURPLE_DRAGON);
                    break;

                case 5:
                    tmp = this->randomMonster(RED_DRAGON, PURPLE_DRAGON);
                    break;

                case 6:
                    tmp = this->randomMonster(PURPLE_DRAGON, PURPLE_DRAGON);
                    break;

                default:
                    tmp = this->randomMonster(PURPLE_DRAGON, PURPLE_DRAGON);
                    break;
            }

            this->frame[0][h].object = tmp.object;
            this->frame[0][h].health = tmp.health;
        }
    }
}

/*
[Game::shiftFrame()]
다음의 알고리즘을 시행합니다.
1. 만약 총알이 0 번째 줄에 존재한다면, 배열에서 제거한다. (BULLET -> NONE)
2. 1 번째 줄부터 (끝 - 1)줄 까지 총알이 존재한다면 한 칸씩 위로 옮겨준다.
3. 몬스터를 한 칸씩 아래로 옮겨준다.
*/
bool Game::shiftFrame(){
    for(int h=0;h<this->printframe->horizontal;h++){ //bullet remove (bullet이 첫 번째 줄에 도달했을 떄)
        if(this->frame[0][h].back->object == BULLET){ //만약 오브젝트 뒤쪽에 또 다른 오브젝트가 있으면
            this->frame[0][h].back->object = NONE; //오브젝트를 삭제한다.
            this->frame[0][h].back->health = H_NONE;
        }
        if(this->frame[0][h].object == BULLET){ //만약 오브젝트가 bullet이면
            this->frame[0][h].object = NONE; //오브젝트를 삭제한다.
            this->frame[0][h].health = H_NONE;
        }
    }

    for(int v=1;v<this->printframe->vertical-1;v++){ //bullet shift
        for(int h=0;h<this->printframe->horizontal;h++){
            if(this->frame[v][h].object == BULLET){ //만약 현재 오브젝트가 bullet이면
                if(this->frame[v-1][h].object > BULLET){ //만약 이전 줄 오브젝트가 몬스터이면
                    this->frame[v][h].health--; //bullet의 체력을 1 감소시킨다.
                    this->frame[v-1][h].health--; //몬스터의 체력을 1 감소시킨다.

                    if(this->frame[v][h].health == H_NONE){ //bullet의 체력이 0이면 배열에서 삭제한다.
                        this->frame[v][h].object = NONE;
                        this->frame[v][h].health = H_NONE;
                    }

                    if(this->frame[v-1][h].health == H_NONE){ //몬스터의 체력이 0이면 배열에서 삭제하고, 점수에 추가한다.
                        this->addScore(this->frame[v-1][h].object); //몬스터에 해당하는 점수를 추가한다.
                        this->frame[v-1][h].object = NONE;
                        this->frame[v-1][h].health = H_NONE;
                    }

                    if(this->frame[v][h].object != NONE){
                        if(this->frame[v-1][h].object != NONE){
                            this->frame[v-1][h].back->object = BULLET; //몬스터 뒤에 bullet 오브젝트를 옮긴다.
                            this->frame[v-1][h].back->health = this->frame[v][h].health;
                        }else{
                            this->frame[v-1][h].object = BULLET; //몬스터 위치에 bullet 오브젝트를 옮긴다.
                            this->frame[v-1][h].health = this->frame[v][h].health;
                        }
                    }
                    this->frame[v][h].object = NONE;
                    this->frame[v][h].health = H_NONE;
                }else{
                    this->frame[v-1][h].object = BULLET;
                    this->frame[v-1][h].health = this->frame[v][h].health;
                    this->frame[v][h].object = NONE;
                    this->frame[v][h].health = H_NONE;
                }
            }else if(this->frame[v][h].back->object == BULLET){ //만약 현재 오브젝트 뒤에 bullet이 있으면
                this->frame[v-1][h].object = BULLET;
                this->frame[v-1][h].health = this->frame[v][h].back->health;
                this->frame[v][h].back->object = NONE;
                this->frame[v][h].back->health = H_NONE;
            }
        }
    }

    if(this->distance % this->FrameClock == 0){ //만약 클럭이 (FrameClock - levelCriteria)의 배수이면
        for(int v=this->printframe->vertical-2;v>=0;v--){
            for(int h=0;h<this->printframe->horizontal;h++){
                if(v == this->printframe->vertical-2){ //만약 현재 행이 (마지막 행 - 1)의 이라면
                    if(this->frame[v][h].object > BULLET && this->frame[v+1][h].object == PLAYER){ //만약 현재 오브젝트가 몬스터(3 이상) 이고 다음 행에 플레이어가 위치하고 있으면 (플레이어와 몬스터가 충돌할 조건)
                        if(this->PlayerHealth > 1){ //만약 플레이어의 체력이 1보다 크다면
                            this->PlayerHealth--; //플레이어의 체력을 1 감소시킨다.
                            this->frame[v+1][h].health = this->PlayerHealth; //체력 감소를 배열에 반영한다.
                        }else{ //플레이어의 체력이 1 이하라면
                            //return false; //게임 오버
                        }
                        continue;
                    }

                    this->frame[v][h].object = NONE;
                    this->frame[v][h].health = H_NONE;
                    continue; //이줄 밑의 코드를 실행하지 않고 바로 다음 반복문을 실행한다.
                }

                if(this->frame[v][h].object == BULLET){ //만약 현재 오브젝트가 bullet이면
                    continue; //이 밑의 코드를 실행하지 않고 바로 다음 반복문을 실행한다. (bullet을 아래 방향으로 shift 하지 않기 위함)
                }
                else if(this->frame[v][h].object > BULLET){ //만약 현재 오브젝트가 몬스터(3 이상) 이면
                    if(this->frame[v+1][h].object == BULLET){ //만약 다음줄에 bullet이 위치하고 있으면
                        if(this->frame[v][h].health > 1){ //만약 몬스터의 체력이 1보다 크면
                            this->frame[v][h].health--; //몬스터의 체력을 1 감소시킨다.
                            this->frame[v+1][h].health--; //bullet의 체력을 1 감소시킨다.
                            if(this->frame[v+1][h].health == H_NONE) this->frame[v+1][h].object = NONE; //만약 bullet의 체력이 0이면 NONE으로 바꿔준다.
                        }else{ //만약 몬스터의 체력이 1 이하이면
                            this->addScore(this->frame[v][h].object); //몬스터에 해당하는 점수를 추가한다.
                            this->frame[v][h].object = NONE; //현재 오브젝트를 제거한다.
                            this->frame[v][h].health = H_NONE;
                            this->frame[v][h].back->object = NONE;
                            this->frame[v][h].back->health = H_NONE;
                            this->frame[v+1][h].health--; //bullet의 체력을 1 감소시킨다.
                            if(this->frame[v+1][h].health == H_NONE) this->frame[v+1][h].object = NONE; //만약 bullet의 체력이 0이면 NONE으로 바꿔준다.
                            continue;
                        }
                    }
                    
                    //몬스터를 다음줄로 이동시킨다.
                    if(this->frame[v+1][h].object == BULLET){ //만약 다음줄에 bullet이 위치하고 있으면
                        this->frame[v+1][h].back->object = this->frame[v+1][h].object; //다음줄의 오브젝트를 back으로 옮긴다.
                        this->frame[v+1][h].back->health = this->frame[v+1][h].health;
                        this->frame[v+1][h].object = this->frame[v][h].object;
                        this->frame[v+1][h].health = this->frame[v][h].health;
                        this->frame[v][h].object = NONE;
                        this->frame[v][h].health = H_NONE;
                    }else{
                        this->frame[v+1][h].object = this->frame[v][h].object;
                        this->frame[v+1][h].health = this->frame[v][h].health;
                        this->frame[v][h].object = NONE;
                        this->frame[v][h].health = H_NONE;
                    }
                }
            }
        }
    }
    return true;
}

void Game::addScore(int target){
    switch(target){
        case WHITE_DRAGON:
            this->score += S_WHITE_DRAGON;
            break;
        
        case YELLOW_DRAGON:
            this->score += S_YELLOW_DRAGON;
            break;

        case GREEN_DRAGON:
            this->score += S_GREEN_DRAGON;
            break;

        case RED_DRAGON:
            this->score += S_RED_DRAGON;
            break;

        case PURPLE_DRAGON:
            this->score += S_PURPLE_DRAGON;
            break;

        default:
            Console::ErrorExit("Error Occured in [Game::addScore()] with error value default");
            break;
    }
}

int Game::SCREENpause(){
    this->printframe->printPause();
    while(1){
        Console::eventStruct event;
        Console::getEvent(&event);
        if(event.eventType == E_KEY_EVENT){
            if(event.keyPressed == true && event.key == PAUSE_KEY){
                break;
            }
        }
    }

    Console::useEventInput(false);
    Console::cls();
    Console::windowSize(this->printframe->consolehorizontal, this->printframe->consolevertical);
    Console::cursorVisible(false);
    this->printframe->printLogo();
    this->printframe->printScoreframe();
    Console::useEventInput(true);
}

void Game::Over(){
    this->printframe->printGameOver();

    //Console::cursorVisible(true);
    //Console::useEventInput(false);
}

/*
[main()]
다음의 알고리즘을 시행합니다.
*/
int main(){
    Game game;
    
    game.init();
    game.makeClock();
}