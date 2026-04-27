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
 - 노란색 : 500m 이후에서 나옵니다. (100점)
 - 초록색 : 1000m 이후에서 나옵니다. (200점)
 - 빨간색 : 1500m 이후에서 나옵니다. (300점)
 - 보라색 : 2000m 이후에서 나옵니다. (500점)

[Meteorite, 운석]
 빨간색 느낌표로 줄이 생성되며 운석이 떨어질 것을 예고합니다.
 운석은 매우 빠른 속력으로 내려오며 1x1 크기입니다.

=== 클래스 설명 ===
class Frame
-> 게임의 화면을 출력하기 위한 클래스입니다.
-> 매 프레임마다 패치 및 출력을 합니다.

class Game
-> 게임의 논리 구현부 입니다.
-> 매 클럭마다 플레이어의 위치를 연산하며, 배열을 출력합니다.
-> 매 프레임 마다 게임 구성 요소(몬스터, 운석, 플레이어와의 충돌 등)를 움직이는 연산을 합니다.
*/

//IO 컨트롤
#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <future>
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>
#include <deque>
#include <mutex>
#include <streambuf>
#include <cstdarg>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <ctime>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
using namespace std;

typedef const char* LPCSTR;

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
#define METEOR 8

//체력 정의
#define H_NONE 0
#define H_PLAYER 3
#define H_BULLET 3
#define H_WHITE_DRAGON 1
#define H_YELLOW_DRAGON 2
#define H_GREEN_DRAGON 3
#define H_RED_DRAGON 4
#define H_PURPLE_DRAGON 5
#define H_METEOR 9

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
#define E_Q_KEY 113
#define E_W_KEY 119
#define E_E_KEY 101

typedef struct Element{
    int object; //자신의 오브젝트 번호
    int health = H_NONE; //자신의 체력
    Element* back; //오브젝트의 뒤에 중첩된 오브젝트
} Element;

//콘솔창 제어 함수
namespace Console{ //콘솔을 제어할 함수들을 모아놓은 이름 공간
    int counter = 0;

    typedef struct xy{ //좌표 반환을 위해 설계한 구조체
        int x;
        int y;
    } xy;

    typedef struct eventStruct{ //마우스와 키보드 이벤트 반환을 위해 설계한 구조체
        int eventType;
        int key;
        bool keyPressed;
        bool Clicked;
        int ClickKey;
        xy coordinate;
    } eventStruct;

    typedef struct Cell{
        uint32_t ch;
        unsigned char text;
        unsigned char back;
        bool continuation;
    } Cell;

    static int consoleWidth = 180;
    static int consoleHeight = 55;
    static int cursorX = 0;
    static int cursorY = 0;
    static int currentText = B_WHITE;
    static int currentBack = BLACK;
    static bool visibleCursor = true;
    static bool eventEnabled = false;
    static bool initialized = false;
    static bool needRender = true;

    static vector<Cell> cells;
    static deque<eventStruct> eventQueue;
    static mutex eventLock;

    static int clampInt(int value, int lower, int upper){
        if(upper < lower) return lower;
        if(lower == upper) return lower;
        if(value < lower) return lower;
        if(value > upper) return upper;
        return value;
    }

    static int frameLeft = 6;
    static int frameTop = 4;
    static int frameWidth = 15;
    static int frameHeight = 25;

    static bool isWithin(int value, int minInclusive, int maxExclusive){
        return value >= minInclusive && value < maxExclusive;
    }

    static int mapRange(int value, int srcMin, int srcMaxExclusive, int dstMin, int dstMaxExclusive){
        int srcRange = srcMaxExclusive - srcMin;
        int dstRange = dstMaxExclusive - dstMin;
        if(srcRange <= 0 || dstRange <= 0) return dstMin;

        int rel = value - srcMin;
        if(rel < 0) rel = 0;
        if(rel >= srcRange) rel = srcRange - 1;

        int mapped = dstMin + (rel * dstRange) / srcRange;
        if(mapped < dstMin) mapped = dstMin;
        if(mapped >= dstMaxExclusive) mapped = dstMaxExclusive - 1;
        return mapped;
    }

    static xy mapCoordinateToConsole(int px, int py, int canvasW, int canvasH){
        if(canvasW <= 0) canvasW = 1;
        if(canvasH <= 0) canvasH = 1;

        xy result;

        int boardStartX = frameLeft + 1;
        int boardEndX = boardStartX + frameWidth;
        int boardStartY = frameTop + 1;
        int boardEndY = boardStartY + frameHeight;

        if(isWithin(px, boardStartX, boardEndX)){
            result.x = mapRange(px, boardStartX, boardEndX, 0, canvasW);
        }else if(isWithin(px, 83, 97)){
            result.x = mapRange(px, 83, 97, 0, canvasW);
        }else if(isWithin(px, 0, consoleWidth)){
            result.x = mapRange(px, 0, consoleWidth, 0, canvasW);
        }else{
            result.x = clampInt((int)((double)px * (double)canvasW / (double)max(1, consoleWidth)), 0, canvasW - 1);
        }

        if(isWithin(py, boardStartY, boardEndY)){
            result.y = mapRange(py, boardStartY, boardEndY, 0, canvasH);
        }else if(isWithin(py, 14, 17)){
            result.y = mapRange(py, 14, 17, 0, canvasH);
        }else if(isWithin(py, 18, 21)){
            result.y = mapRange(py, 18, 21, 0, canvasH);
        }else if(isWithin(py, 22, 25)){
            result.y = mapRange(py, 22, 25, 0, canvasH);
        }else if(isWithin(py, 35, 38)){
            result.y = mapRange(py, 35, 38, 0, canvasH);
        }else if(isWithin(py, 39, 42)){
            result.y = mapRange(py, 39, 42, 0, canvasH);
        }else if(isWithin(py, 0, consoleHeight)){
            result.y = mapRange(py, 0, consoleHeight, 0, canvasH);
        }else{
            result.y = clampInt((int)((double)py * (double)canvasH / (double)max(1, consoleHeight)), 0, canvasH - 1);
        }

        return result;
    }

    static xy mapCoordinateFromCanvas(int cx, int cy, int canvasW, int canvasH){
        if(canvasW <= 0) canvasW = 1;
        if(canvasH <= 0) canvasH = 1;

        xy result;
        result.x = clampInt(mapRange(cx, 0, canvasW, 0, consoleWidth), 0, consoleWidth - 1);
        result.y = clampInt(mapRange(cy, 0, canvasH, 0, consoleHeight), 0, consoleHeight - 1);
        return result;
    }

    static void ensureCellBuffer(){
        if(consoleWidth < 1) consoleWidth = 1;
        if(consoleHeight < 1) consoleHeight = 1;
        cells.assign(consoleWidth * consoleHeight, Cell{(uint32_t)' ', (unsigned char)B_WHITE, (unsigned char)BLACK, false});
        cursorX = clampInt(cursorX, 0, consoleWidth - 1);
        cursorY = clampInt(cursorY, 0, consoleHeight - 1);
        needRender = true;
    }

    static void clearEvents(){
        lock_guard<mutex> lock(eventLock);
        eventQueue.clear();
    }

    static void pushEvent(const eventStruct& event){
        lock_guard<mutex> lock(eventLock);
        eventQueue.push_back(event);
    }

    static bool popEvent(eventStruct* event){
        lock_guard<mutex> lock(eventLock);
        if(eventQueue.empty()) return false;
        *event = eventQueue.front();
        eventQueue.pop_front();
        return true;
    }

#ifdef __EMSCRIPTEN__
    EM_JS(void, webConsoleSetup, (), {
        var canvas = Module['canvas'];
        if (!canvas) {
            canvas = document.createElement('canvas');
            canvas.id = 'canvas';
            document.body.appendChild(canvas);
            Module['canvas'] = canvas;
        }

        canvas.width = 1600;
        canvas.height = 900;
        canvas.style.width = '100vw';
        canvas.style.maxWidth = '1600px';
        canvas.style.height = 'auto';
        canvas.style.display = 'block';
        canvas.style.margin = '0 auto';
        canvas.style.background = '#000000';
        canvas.style.imageRendering = 'pixelated';
        canvas.style.outline = 'none';
        canvas.style.boxShadow = 'none';
        canvas.tabIndex = -1;

        document.body.style.margin = '0';
        document.body.style.background = '#000000';

        var notice = document.getElementById('dragon-flight-web-notice');
        if (!notice) {
            notice = document.createElement('div');
            notice.id = 'dragon-flight-web-notice';
            notice.style.color = '#c9d1d9';
            notice.style.fontFamily = 'monospace';
            notice.style.fontSize = '14px';
            notice.style.maxWidth = '1600px';
            notice.style.margin = '12px auto';
            notice.style.padding = '0 10px';
            notice.textContent = 'Dragon Flight Web - 마우스로 이동, Q/W/E 키 입력';
            var parent = canvas.parentNode || document.body;
            parent.insertBefore(notice, canvas);
        }

    });

    EM_JS(int, webCanvasClientWidth, (), {
        var canvas = Module['canvas'];
        if (!canvas) return 0;
        var rect = canvas.getBoundingClientRect();
        return Math.max(1, Math.round(rect.width || canvas.clientWidth || canvas.width || 0));
    });

    EM_JS(int, webCanvasClientHeight, (), {
        var canvas = Module['canvas'];
        if (!canvas) return 0;
        var rect = canvas.getBoundingClientRect();
        return Math.max(1, Math.round(rect.height || canvas.clientHeight || canvas.height || 0));
    });

    EM_JS(void, webConsoleRender,
          (int width, int height, const uint32_t* chars, const unsigned char* text, const unsigned char* back,
           const unsigned char* continuation, int cursorVisible, int cx, int cy), {
        var canvas = Module['canvas'];
        if (!canvas) return;

        var ctx = canvas.getContext('2d');
        if (!ctx) return;

        var total = width * height;
        var heapChar = HEAPU32.subarray(chars >> 2, (chars >> 2) + total);
        var heapText = HEAPU8.subarray(text, text + total);
        var heapBack = HEAPU8.subarray(back, back + total);
        var heapContinuation = HEAPU8.subarray(continuation, continuation + total);

        var palette = [
            '#000000', '#000080', '#008000', '#008080', '#800000', '#800080', '#808000', '#c0c0c0',
            '#808080', '#5555ff', '#55ff55', '#55ffff', '#ff5555', '#ff55ff', '#ffff55', '#ffffff'
        ];

        var cw = canvas.width / width;
        var ch = canvas.height / height;

        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.textBaseline = 'top';
        ctx.textAlign = 'left';
        var fontPx = Math.max(8, Math.min(ch - 2, cw * 1.6));
        ctx.font = fontPx + 'px "Nanum Gothic Coding Web", "Nanum Gothic Coding", "D2Coding", "Noto Sans Mono CJK KR", "Malgun Gothic", "Apple SD Gothic Neo", "Courier New", Consolas, monospace';

        for (var y = 0; y < height; y++) {
            for (var x = 0; x < width; x++) {
                var idx = y * width + x;
                var px = x * cw;
                var py = y * ch;
                ctx.fillStyle = palette[heapBack[idx] & 15];
                ctx.fillRect(px, py, Math.ceil(cw), Math.ceil(ch));
            }
        }

        for (var y = 0; y < height; y++) {
            for (var x = 0; x < width; x++) {
                var idx = y * width + x;
                var px = x * cw;
                var py = y * ch;

                if (heapContinuation[idx]) {
                    continue;
                }

                var codepoint = heapChar[idx];
                if (codepoint !== 0 && codepoint !== 32) {
                    ctx.fillStyle = palette[heapText[idx] & 15];
                    var isWide = codepoint >= 0x1100 && (
                        (codepoint <= 0x115F) ||
                        (codepoint >= 0x2329 && codepoint <= 0x232A) ||
                        (codepoint >= 0x2E80 && codepoint <= 0xA4CF) ||
                        (codepoint >= 0xAC00 && codepoint <= 0xD7A3) ||
                        (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||
                        (codepoint >= 0xFE10 && codepoint <= 0xFE19) ||
                        (codepoint >= 0xFE30 && codepoint <= 0xFE6F) ||
                        (codepoint >= 0xFF01 && codepoint <= 0xFF60) ||
                        (codepoint >= 0xFFE0 && codepoint <= 0xFFE6)
                    );
                    var drawText = String.fromCodePoint(codepoint);
                    var drawWidth = isWide ? cw * 2 : cw;
                    var measured = ctx.measureText(drawText).width || drawWidth;
                    var scaleX = measured > 0 ? Math.min(1, drawWidth / measured) : 1;
                    var offsetX = Math.max(0, (drawWidth - measured * scaleX) / 2);
                    ctx.save();
                    ctx.translate(px + offsetX, py);
                    ctx.scale(scaleX, 1);
                    ctx.fillText(drawText, 0, 0);
                    ctx.restore();
                }
            }
        }

        if (cursorVisible) {
            ctx.strokeStyle = '#ffffff';
            ctx.lineWidth = 1;
            ctx.strokeRect(cx * cw + 1, cy * ch + 1, Math.max(1, cw - 2), Math.max(1, ch - 2));
        }
    });

    static xy mapMouseCoordinate(int x, int y){
        int canvasW = 1600;
        int canvasH = 900;
        int clientW = 0;
        int clientH = 0;
        emscripten_get_canvas_element_size("#canvas", &canvasW, &canvasH);
        clientW = webCanvasClientWidth();
        clientH = webCanvasClientHeight();
        if(clientW > 0) canvasW = clientW;
        if(clientH > 0) canvasH = clientH;
        if(canvasW < 1) canvasW = 1;
        if(canvasH < 1) canvasH = 1;

        return mapCoordinateFromCanvas(x, y, canvasW, canvasH);
    }

    static EM_BOOL mouseMoveCallback(int, const EmscriptenMouseEvent* e, void*){
        if(eventEnabled == false) return EM_TRUE;
        eventStruct event = {E_MOUSE_EVENT, 0, false, false, false, {0, 0}};
        int mouseX = (e->targetX != 0 || e->targetY != 0) ? e->targetX : e->canvasX;
        int mouseY = (e->targetX != 0 || e->targetY != 0) ? e->targetY : e->canvasY;
        event.coordinate = mapMouseCoordinate(mouseX, mouseY);
        pushEvent(event);
        return EM_TRUE;
    }

    static EM_BOOL mouseDownCallback(int, const EmscriptenMouseEvent* e, void*){
        if(eventEnabled == false) return EM_TRUE;
        eventStruct event = {E_MOUSE_EVENT, 0, false, true, false, {0, 0}};
        int mouseX = (e->targetX != 0 || e->targetY != 0) ? e->targetX : e->canvasX;
        int mouseY = (e->targetX != 0 || e->targetY != 0) ? e->targetY : e->canvasY;
        event.coordinate = mapMouseCoordinate(mouseX, mouseY);
        event.ClickKey = (e->button == 2) ? E_MOUSE_RIGHT : E_MOUSE_LEFT;
        pushEvent(event);
        return EM_TRUE;
    }

    static EM_BOOL keyDownCallback(int, const EmscriptenKeyboardEvent* e, void*){
        if(eventEnabled == false) return EM_TRUE;
        if(e->repeat) return EM_TRUE;

        int key = e->which ? e->which : e->keyCode;
        if(key >= 65 && key <= 90) key += 32;
        eventStruct event = {E_KEY_EVENT, key, true, false, false, {0, 0}};
        pushEvent(event);
        return EM_TRUE;
    }

    static EM_BOOL keyUpCallback(int, const EmscriptenKeyboardEvent* e, void*){
        if(eventEnabled == false) return EM_TRUE;

        int key = e->which ? e->which : e->keyCode;
        if(key >= 65 && key <= 90) key += 32;
        eventStruct event = {E_KEY_EVENT, key, false, false, false, {0, 0}};
        pushEvent(event);
        return EM_TRUE;
    }
#endif

    static void scrollUp(){
        for(int y=1;y<consoleHeight;y++){
            for(int x=0;x<consoleWidth;x++){
                cells[(y-1) * consoleWidth + x] = cells[y * consoleWidth + x];
            }
        }

        for(int x=0;x<consoleWidth;x++){
            cells[(consoleHeight - 1) * consoleWidth + x] = Cell{(uint32_t)' ', (unsigned char)currentText, (unsigned char)currentBack, false};
        }
        cursorY = consoleHeight - 1;
    }

    static bool isWideCodepoint(uint32_t cp){
        return
            (cp >= 0x1100 && cp <= 0x115F) ||
            (cp >= 0x2329 && cp <= 0x232A) ||
            (cp >= 0x2E80 && cp <= 0xA4CF) ||
            (cp >= 0xAC00 && cp <= 0xD7A3) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0xFE10 && cp <= 0xFE19) ||
            (cp >= 0xFE30 && cp <= 0xFE6F) ||
            (cp >= 0xFF01 && cp <= 0xFF60) ||
            (cp >= 0xFFE0 && cp <= 0xFFE6) ||
            (cp >= 0x1F300 && cp <= 0x1FAFF);
    }

    static void putCodepoint(uint32_t cp){
        if(cp == '\r'){
            cursorX = 0;
            return;
        }
        if(cp == '\n'){
            cursorX = 0;
            cursorY++;
            if(cursorY >= consoleHeight) scrollUp();
            needRender = true;
            return;
        }
        if(cp == '\t'){
            for(int i=0;i<4;i++) putCodepoint(' ');
            return;
        }
        if(cp < 32) return;

        int cellWidth = isWideCodepoint(cp) ? 2 : 1;

        if(cursorX >= consoleWidth || (cellWidth == 2 && cursorX >= consoleWidth - 1)){
            cursorX = 0;
            cursorY++;
            if(cursorY >= consoleHeight) scrollUp();
        }

        if(cursorX >= 0 && cursorX < consoleWidth && cursorY >= 0 && cursorY < consoleHeight){
            Cell& cell = cells[cursorY * consoleWidth + cursorX];
            cell.ch = cp;
            cell.text = (unsigned char)(currentText & 15);
            cell.back = (unsigned char)(currentBack & 15);
            cell.continuation = false;
            needRender = true;

            if(cellWidth == 2 && cursorX + 1 < consoleWidth){
                Cell& nextCell = cells[cursorY * consoleWidth + cursorX + 1];
                nextCell.ch = (uint32_t)' ';
                nextCell.text = (unsigned char)(currentText & 15);
                nextCell.back = (unsigned char)(currentBack & 15);
                nextCell.continuation = true;
            }
        }

        cursorX += cellWidth;
    }

    static void putUtf8(const char* data, size_t len){
        size_t i = 0;
        while(i < len){
            unsigned char c0 = (unsigned char)data[i];
            if(c0 < 0x80){
                putCodepoint(c0);
                i++;
                continue;
            }

            uint32_t cp = '?';
            int extra = 0;
            if((c0 & 0xE0) == 0xC0 && i + 1 < len){
                cp = ((uint32_t)(c0 & 0x1F) << 6) |
                     ((uint32_t)((unsigned char)data[i+1] & 0x3F));
                extra = 1;
            }else if((c0 & 0xF0) == 0xE0 && i + 2 < len){
                cp = ((uint32_t)(c0 & 0x0F) << 12) |
                     ((uint32_t)((unsigned char)data[i+1] & 0x3F) << 6) |
                     ((uint32_t)((unsigned char)data[i+2] & 0x3F));
                extra = 2;
            }else if((c0 & 0xF8) == 0xF0 && i + 3 < len){
                cp = ((uint32_t)(c0 & 0x07) << 18) |
                     ((uint32_t)((unsigned char)data[i+1] & 0x3F) << 12) |
                     ((uint32_t)((unsigned char)data[i+2] & 0x3F) << 6) |
                     ((uint32_t)((unsigned char)data[i+3] & 0x3F));
                extra = 3;
            }

            putCodepoint(cp);
            i += (size_t)extra + 1;
        }
    }

    class ConsoleStreamBuf : public std::streambuf{
        protected:
            int_type overflow(int_type ch) override{
                if(ch != traits_type::eof()){
                    char c = (char)ch;
                    putUtf8(&c, 1);
                }
                return ch;
            }

            std::streamsize xsputn(const char* s, std::streamsize n) override{
                if(n > 0) putUtf8(s, (size_t)n);
                return n;
            }
    };

    static ConsoleStreamBuf consoleStreamBuf;
    static std::streambuf* originalCoutBuf = nullptr;

#ifdef __EMSCRIPTEN__
    static vector<uint32_t> renderChars;
    static vector<unsigned char> renderText;
    static vector<unsigned char> renderBack;
    static vector<unsigned char> renderContinuation;
#endif

    void present(){
        if(initialized == false || needRender == false) return;

#ifdef __EMSCRIPTEN__
        size_t total = cells.size();
        if(total > 0){
            renderChars.resize(total);
            renderText.resize(total);
            renderBack.resize(total);
            renderContinuation.resize(total);

            for(size_t i=0;i<total;i++){
                renderChars[i] = cells[i].ch;
                renderText[i] = cells[i].text;
                renderBack[i] = cells[i].back;
                renderContinuation[i] = cells[i].continuation ? 1 : 0;
            }

            webConsoleRender(consoleWidth, consoleHeight,
                             &renderChars[0],
                             &renderText[0],
                             &renderBack[0],
                             &renderContinuation[0],
                             visibleCursor ? 1 : 0,
                             cursorX, cursorY);
        }
#else
        cout.flush();
#endif
        needRender = false;
    }

    void cls();

    int printfCompat(const char* format, ...){
        char local[4096];

        va_list args;
        va_start(args, format);
        int count = vsnprintf(local, sizeof(local), format, args);
        va_end(args);

        if(count < 0) return count;

        if((size_t)count >= sizeof(local)){
            vector<char> dynamicBuffer((size_t)count + 1);
            va_start(args, format);
            vsnprintf(&dynamicBuffer[0], dynamicBuffer.size(), format, args);
            va_end(args);
            putUtf8(&dynamicBuffer[0], (size_t)count);
        }else{
            putUtf8(local, (size_t)count);
        }

        return count;
    }

    void init(){ //인코딩을 UTF-8로 바꾸고, 콘솔창 제목을 설정
        if(initialized == true) return;

        ensureCellBuffer();
        originalCoutBuf = cout.rdbuf(&consoleStreamBuf);

#ifdef __EMSCRIPTEN__
        webConsoleSetup();
        emscripten_set_mousemove_callback("#canvas", nullptr, EM_TRUE, mouseMoveCallback);
        emscripten_set_mousedown_callback("#canvas", nullptr, EM_TRUE, mouseDownCallback);
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, keyDownCallback);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, keyUpCallback);
#endif

        initialized = true;
        cls();
        present();
    }

    void configureFrameRegion(int left, int top, int width, int height){
        frameLeft = left;
        frameTop = top;
        frameWidth = (width > 0) ? width : 1;
        frameHeight = (height > 0) ? height : 1;
    }

    xy toCanvasCoordinate(xy consoleCoordinate){
        int canvasW = 1600;
        int canvasH = 900;
#ifdef __EMSCRIPTEN__
        emscripten_get_canvas_element_size("#canvas", &canvasW, &canvasH);
#endif
        return mapCoordinateToConsole(consoleCoordinate.x, consoleCoordinate.y, canvasW, canvasH);
    }


    void sleep(float sec){ //sec초 만큼 정지
        present();
        if(sec < 0.0f) sec = 0.0f;
        int ms = (int)(sec * 1000.0f);
        if(ms < 1) ms = 1;

#ifdef __EMSCRIPTEN__
        emscripten_sleep(ms);
#else
        this_thread::sleep_for(chrono::milliseconds(ms));
#endif
    }

    void cls(){ //화면을 초기화
        for(size_t i=0;i<cells.size();i++){
            cells[i].ch = ' ';
            cells[i].text = (unsigned char)currentText;
            cells[i].back = (unsigned char)currentBack;
        }
        cursorX = 0;
        cursorY = 0;
        needRender = true;
    }

    void windowSize(int x, int y){ //윈도우 사이즈를 바꿈
        consoleWidth = (x > 1) ? x : 1;
        consoleHeight = (y > 1) ? y : 1;
        ensureCellBuffer();
        cls();
    }

    void cursorVisible(bool status){ //커서의 표시 여부
        visibleCursor = status;
        needRender = true;
    }

    void gotoxy(int x, int y){ //커서를 (x, y)로 이동
        cursorX = clampInt(x, 0, consoleWidth - 1);
        cursorY = clampInt(y, 0, consoleHeight - 1);
    }

    void setColor(int text, int back){ //글자색과 배경색을 바꿈
        currentText = text;
        currentBack = back;
    }

    void ErrorExit(LPCSTR lpszMessage){ //lpszMessage를 출력 후 프로그램 종료
        fprintf(stderr, "%s\n", lpszMessage);
        present();
        exit(0);
    }

    void useEventInput(bool status){ //이벤트 입력(마우스 / 키보드)을 사용할지 결정
        eventEnabled = status;
        if(eventEnabled == false) clearEvents();
    }

    bool pollEvent(eventStruct* event){
        event->eventType = NONE;
        event->key = 0;
        event->keyPressed = false;
        event->Clicked = false;
        event->ClickKey = false;
        event->coordinate = {0, 0};

        if(eventEnabled == false) return false;
        return popEvent(event);
    }
    
    void getEvent(eventStruct* event){ //발생한 이벤트를 event에 저장
        event->eventType = NONE;
        event->key = 0;
        event->keyPressed = false;
        event->Clicked = false;
        event->ClickKey = false;
        event->coordinate = {0, 0};

        if(eventEnabled == false) return;

        while(popEvent(event) == false){
            present();
#ifdef __EMSCRIPTEN__
            emscripten_sleep(1);
#else
            this_thread::sleep_for(chrono::milliseconds(1));
#endif
        }
    }

    void waitEvent(promise<eventStruct> *p){ //이벤트를 기다릴 스레드 함수
        eventStruct event;
        event.eventType = NONE;
        while(event.eventType == NONE){
            getEvent(&event);
        }
        p->set_value(event);
    }

    void moveWindowCenter(){ //콘솔창을 화면 정가운데로 옮김
    }

    void moveWindowCoordinate(int x, int y){ //콘솔창을 죄측 상단을 기준으로 (x, y)로 이동
        (void)x;
        (void)y;
    }
}

#define printf Console::printfCompat

//=================================== 메인 코드 ===================================

class Frame{
    public:
        //frame 설정
        int fps; //초당 프레임
        int horizontal; //가로
        int vertical; //세로
        Element **frame; //frame 포인터
        double interval; //fps에 따른 frame갱신 시간

        int consolehorizontal; //콘솔창 가로
        int consolevertical; //콘솔창 세로

        void print(); //게임 화면 프린트
        void printAlert(int alertcode); //인내 메세지 프린트
        void printIntro(); //인트로 프린트
        void printMain(); //메인 화면 프린트
        void printPause(); //일시정지 화면 프린트
        void printBlank(); //빈 틀을 프린트
        void printLogo(); //로고 프린트
        void printColorLine(int textcolor, int backcolor, int horizontal); //컬러 라인을 프린트
        void printScore(int score, int distance, int level, int levelCriteria, int PlayerHealth); //점수 프린트
        void printScoreframe(); //점수 프레임 프린트
        void printGameOver(int score, int distance, int level); //게임 오버 화면 프린트
        int SkipFramePer = 1; //출력할 프레임 배수(나머지 프레임은 출력을 하지 않음)
        int LogoVertical = 3; //로고 세로 길이
        int LeftSpace = 6; //게임 배열 좌측 공간
        int UpperSpace = 4; //게임 배열 위쪽 공간
        int ScoreboardHeight = 4; //점수 프레임을 프린트할 때 위의 공간
        int alertcode; //알림 코드

        Frame(int fps, int horizontal, int vertical); //생성자
};

Frame::Frame(int fps, int horizontal, int vertical){
    this->fps = fps;
    this->horizontal = horizontal;
    this->vertical = vertical;
    this->interval = 1.0/(float)(this->fps);
    this->alertcode = 0;

    this->frame = new Element *[this->vertical];
    for(int i=0;i < this->vertical;i++){
        this->frame[i] = new Element [this->horizontal];
    }

    Console::init();
}

/*
[Frame::print()]
다음의 알고리즘을 시행합니다.
1. frame 배열을 프린트합니다. (배경색은 Element, 숫자는 체력을 의미한다.)
*/
void Frame::print(){ //게임중 배열을 프린트
    Console::gotoxy(0, this->UpperSpace);

    if(this->alertcode == 2) Console::setColor(B_RED, BLACK);
    else Console::setColor(B_WHITE, BLACK);
    Console::gotoxy(this->LeftSpace, this->UpperSpace);
    printf("┌");
    for(int v=0;v<this->horizontal;v++) printf("─");
    printf("┐\n");

    int v;
    for(v=0;v<this->vertical;v++){
        Console::gotoxy(this->LeftSpace, this->UpperSpace+v+1);
        if(this->alertcode == 2) Console::setColor(B_RED, BLACK);
        else Console::setColor(B_WHITE, BLACK);
        printf("│");
        Console::setColor(B_WHITE, BLACK);

        for(int h=0;h<this->horizontal;h++){
            if(this->frame[v][h].object == NONE){
                Console::setColor(B_WHITE, BLACK);
                printf(" ");
            }
            else if(this->frame[v][h].object == METEOR){
                Console::setColor(B_RED, B_RED);
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
        if(this->alertcode == 2) Console::setColor(B_RED, BLACK);
        else Console::setColor(B_WHITE, BLACK);
        printf("│");
        Console::setColor(B_WHITE, BLACK);
    }

    Console::gotoxy(this->LeftSpace, this->UpperSpace+v+1);
    if(this->alertcode == 2) Console::setColor(B_RED, BLACK);
    else Console::setColor(B_WHITE, BLACK);
    printf("└");
    for(int v=0;v<this->horizontal;v++) printf("─");
    printf("┘\n");
    Console::setColor(B_WHITE, BLACK);
}

void Frame::printAlert(int alertcode){
    switch(alertcode){
    case 0:
        Console::gotoxy((this->consolehorizontal - 30)/2, this->consolevertical - 1);
        cout << "                                                                    ";
        this->alertcode = 0;
        break;

    case 1:
        Console::gotoxy((this->consolehorizontal - 30)/2, this->consolevertical - 1);
        Console::setColor(B_WHITE, BLACK);
        cout << "[W]를 눌러 일지정지 할 수 있습니다.         ";
        this->alertcode = 1;
        break;

    case 2:
        Console::gotoxy((this->consolehorizontal - 32)/2, this->consolevertical - 1);
        Console::setColor(B_RED, BLACK);
        cout << "마우스를 플레이 범위 안으로 옮겨주세요!      ";
        Console::setColor(B_WHITE, BLACK);
        this->alertcode = 2;
        break;
    
    default:
        break;
    }
}

void Frame::printLogo(){ //게임중 로고를 프린트
    int nowline = 0;
    string line;
    fstream logo;
    logo.open("assets/logo/MAINLOGO", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy((this->consolehorizontal - 150) * 2, nowline++);
        cout << line << endl;
    }
}

void Frame::printScoreframe(){ //게임중 스코어보드 틀을 프린트
    Console::gotoxy(this->horizontal * 2, this->ScoreboardHeight);
    printf("┌");
    for(int h=0;h<22;h++) printf("─");
    printf("┐");

    int v;
    for(v=0;v<7;v++){
        Console::gotoxy(this->horizontal * 2, this->ScoreboardHeight + 1 + v);
        printf("│");
        for(int h=0;h<22;h++) printf(" ");
        printf("│");
    }

    Console::gotoxy(this->horizontal * 2, this->ScoreboardHeight + 1 + v);
    printf("└");
    for(int v=0;v<22;v++) printf("─");
    printf("┘\n");
}


void Frame::printScore(int score, int distance, int level, int levelCriteria, int PlayerHealth){ //게임중 스코어보드 내에 내용을 출력
    Console::gotoxy((this->horizontal + 1) * 2, this->ScoreboardHeight + 2);
    printf("점수 : %d점", score);
    
    Console::gotoxy((this->horizontal + 1) * 2, this->ScoreboardHeight + 3);
    printf("거리 : %dm", distance);

    Console::gotoxy((this->horizontal + 1) * 2, this->ScoreboardHeight + 4);
    printf("체력 : ");
    Console::setColor(B_RED, BLACK);
    for(int i=0;i<PlayerHealth;i++) printf("❤ ");
    for(int i=0;i<H_PLAYER - PlayerHealth;i++) printf("  ");
    Console::setColor(B_WHITE, BLACK);

    Console::gotoxy((this->horizontal + 1) * 2, this->ScoreboardHeight + 5);
    printf("페이즈 : %d번째 ", level + 1);

    Console::gotoxy((this->horizontal + 1) * 2, this->ScoreboardHeight + 6);
    printf("현재 페이즈 [%.1lf%%] ", ((double)(distance % levelCriteria)/(double)levelCriteria)*(double)100);
}

void Frame::printMain(){ //메인 화면을 출력
    Console::windowSize(this->consolehorizontal,  this->consolevertical + 20);
    Console::moveWindowCenter();
    Console::gotoxy(0, 0);
    printf("┌");
    for(int i=0;i<this->consolehorizontal - 3;i++) printf("─");
    printf("┐");

    for(int i=0;i<this->consolevertical + 18;i++){
        Console::gotoxy(0, i+1);
        printf("│");
        for(int j=0;j<this->consolehorizontal - 3;j++) printf(" ");
        printf("│");
    }

    Console::gotoxy(0, this->consolevertical + 19);
    printf("└");
    for(int i=0;i<this->consolehorizontal - 3;i++) printf("─");
    printf("┘");

    int nowline = 0;
    string line;
    fstream logo;
    logo.open("assets/logo/MAINLOGO", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy((this->consolehorizontal - 100) / 2, 4 + nowline++);
        cout << line << endl;
    }

    Console::gotoxy((this->consolehorizontal - 14) / 2, this->consolevertical + 1);
    printf("시작하기 [Q]");

    Console::gotoxy((this->consolehorizontal - 14) / 2, this->consolevertical + 5);
    printf("종료하기 [W]");

    Console::gotoxy((this->consolehorizontal - 14) / 2, this->consolevertical + 9);
    printf("튜토리얼 [E]");
}

void Frame::printPause(){ //일시정지 화면을 출력
    Console::moveWindowCenter();
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
    logo.open("assets/logo/PAUSELOGO", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy((this->consolehorizontal - 42) / 2, 4 + nowline++);
        cout << line << endl;
    }

    Console::gotoxy((this->consolehorizontal - 14) / 2, 15);
    printf("종료하기 [Q]");

    Console::gotoxy((this->consolehorizontal - 14) / 2, 19);
    printf("복귀하기 [W]");

    Console::gotoxy((this->consolehorizontal - 14) / 2, 23);
    printf("다시시작 [E]");
}

void Frame::printBlank(){ //아무것도 없는 화면을 출력
    Console::gotoxy(0, 0);

    for(int i=0;i<this->consolevertical;i++){
        for(int j=0;j<this->consolehorizontal - 1;j++) printf(" ");
        printf("\n");
    }
}

void Frame::printColorLine(int textcolor, int backcolor, int horizontal){ //운석이 떨어질 것을 알리는 선을 출력
    Console::setColor(textcolor, backcolor);

    for(int i=0;i<this->vertical - 1;i++){
        Console::gotoxy(this->LeftSpace + 1 + horizontal, this->UpperSpace + 1 + i);
        printf(" ! ");
    }

    Console::setColor(B_WHITE, BLACK);
}

void Frame::printGameOver(int score, int distance, int level){ //게임 오버 화면을 출력
    Console::moveWindowCenter();
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
    logo.open("assets/logo/GAMEOVERLOGO", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy((this->consolehorizontal - 74) / 2, 4 + nowline++);
        cout << line << endl;
    }

    Console::gotoxy((this->consolehorizontal - 14) / 2, 15);
    printf("다시시작 [Q]");

    Console::gotoxy((this->consolehorizontal - 14) / 2, 19);
    printf("종료하기 [W]");

    Console::gotoxy((this->consolehorizontal - 14) / 2, 23);
    printf("메인화면 [E]");

    Console::gotoxy((this->consolehorizontal - 14) / 2, 25);
    printf("간 거리 : %dm", distance);
    
    Console::gotoxy((this->consolehorizontal - 14) / 2, 27);
    printf("총 점수 : %d점", score);

    Console::gotoxy((this->consolehorizontal - 14) / 2, 29);
    printf("%d 페이즈 달성!", level);
}

void Frame::printIntro(){ //게임 시작시 인트로 출력
    Console::windowSize(166, 47);
    Console::moveWindowCenter();
    for(int i=82;i>0;i--){
        int nowline = 0;
        string line;
        fstream logo;
        string filename = "assets/intro/" + to_string(i) + ".txt";
        Console::gotoxy(0, 0);
        logo.open(&filename[0], fstream::in);
        while (getline(logo, line)){
            cout << line << endl;
        }
        Console::sleep(1.0f / 60.0f);
    }

    int nowline = 0;
    string line;
    fstream logo;
    logo.open("assets/logo/MAINTAINERLOGO", fstream::in);
    while (getline(logo, line))
    {
        Console::gotoxy((166 - 75) / 2, 47/2 + nowline++);
        cout << line << endl;
    }

    Console::sleep(2.5);
}

class Game{
    public:
        Frame *printframe; //Frame 클래스 포인터
        Element **frame; //frame 포인터
        int distance; //현재 거리
        int level; //현재 레벨(levelCriteria의 배수마다 1 증가)
        int levelCriteria; //한 레벨을 올리는 데의 기준

        int SCREENmain(); //메인 화면이 호출될때 실행되는 함수
        int SCREENpause(); //게임을 일시정지할 때 실행되는 함수
        int SCREENover(); //게임 오버시 실행되는 함수

        int nowAlertcode; //현재 Alertcode
        int PlayerHorizontal; //플레이어의 가로 위치
        int meteorHorizontal; //운석의 가로 위치
        int FrameClock; //프레임을 갱신할 클럭 배수
        int patchMonsterClock; //몬스터를 패치할 클럭 배수 
        int bulletClock; //총알을 패치할 클럭 배수
        int meteorClock; //운석을 패치할 클럭 배수
        bool meteorWarningVisible; //운석 경고선을 현재 프레임 위에 표시할지 여부
        int meteorWarningBackColor;
        int makeClock(); //연산 클럭을 생성함
        bool updateFrame(); //배열을 조작함
        void patchPlayer(Console::xy coor); //플레이어의 가로 위치를 프레임에 패치
        void patchMonster(); //프레임의 맨 윗줄에 몬스터를 패치
        bool shiftFrame(); //맨 윗줄부터 플레이어 이전 줄을 한 칸 아래로 민다.
        void printFrame(); //매 클럭당 출력

        Console::xy toCanvasCoordinate(Console::xy coor); //콘솔 좌표를 캔버스 좌표로 변환

        void addScore(int target); //몬스터에 따라 score변수에 점수 추가
        Element randomMonster(int from, int to); //from부터 to까지 범위에 있는 몬스터를 랜덤으로 뽑아 구조체를 반환
        int score; //플레이어의 점수
        int PlayerHealth; //플레이어의 체력

        void init(); //새 게임 시작 전 초기화자
        Game(); //생성자
};

Game::Game(){ //생성자 : 메인 함수에서 클래스를 선언할 때 선언하자마자 호출없이 바로 살행되는 함수
    this->printframe = new Frame(30, 15, 25); //frame 배열을 프린트하고, 관리할 Frame 클래스를 printframe이라는 이름으로 선언
    this->frame = this->printframe->frame; //game의 frame과 printframe의 frame이 같은 배열을 가르키도록 주소를 복사

    this->levelCriteria = 500; //한 레벨을 올리는 데의 기준
    this->FrameClock = 10; //FrameClock의 배수 클럭마다 프레임이 갱신된다.
    this->patchMonsterClock = 120; //patchMonsterClock의 배수 클럭마다 몬스터가 맨 윗줄에 패치된다.
    this->bulletClock = 4; //bulletClock의 배수 클럭마다 플레이어 바로 윗줄에 bullet이 생성이 된다.
    this->meteorClock = 100;
    this->meteorWarningVisible = false;
    this->meteorWarningBackColor = B_RED;

    this->printframe->consolevertical = this->printframe->vertical + 10; //콘솔창의 세로 길이
    this->printframe->consolehorizontal = this->printframe->horizontal + 170; //콘솔창의 가로 길이
    Console::useEventInput(true); //마우스 사용을 선언한다.
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
    this->meteorHorizontal = this->PlayerHorizontal;
    this->frame[this->printframe->vertical-1][this->PlayerHorizontal].object = PLAYER; //PlayerHorizontal 칸을 PLAYER로 지정한다.
    this->frame[this->printframe->vertical-1][this->PlayerHorizontal].health = this->PlayerHealth; //플레이어의 체력을 설정한다.
    this->distance = 1; //현재 거리 (0으로 시작하면 첫 운석이 경고 없이 바로 생성되므로 1부터 시작)
    this->meteorWarningVisible = false;
    this->meteorWarningBackColor = B_RED;
    this->level = 0; //현재 난이도
    this->score = 0; //점수
    this->printframe->SkipFramePer = 1;
    
    srand(time(NULL)); //난수 시드 설정
    Console::windowSize(this->printframe->consolehorizontal, this->printframe->consolevertical); //윈도우 사이즈를 바꾼다.
    Console::moveWindowCenter();
    Console::cls(); //화면을 초기화
    Console::cursorVisible(false); //커서를 보이지 않게 한다.
    this->printframe->printLogo(); //로고를 프린트한다.
    this->printframe->printAlert(1); //안내 메시지를 프린트한다.
    this->nowAlertcode = 1;
    this->printframe->printScoreframe(); //점수판 위치에 틀을 프린트한다.
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
int Game::makeClock(){
    bool gameStatus = true; //gameStatus을 true로 초기화
    while(1){ //게임이 종료될 때 까지 반복
#ifdef __EMSCRIPTEN__
        double tickMs = this->printframe->interval * 1000.0;
        double accumulatorMs = 0.0;
        double lastTimeMs = emscripten_get_now();
        while (gameStatus) { //gameStatus가 false가 아니면 계속 반복한다.
            double nowTimeMs = emscripten_get_now();
            double elapsedMs = nowTimeMs - lastTimeMs;
            lastTimeMs = nowTimeMs;

            if(elapsedMs < 0.0) elapsedMs = 0.0;
            if(elapsedMs > tickMs * 5.0) elapsedMs = tickMs * 5.0;
            accumulatorMs += elapsedMs;
            if(accumulatorMs > tickMs * 5.0) accumulatorMs = tickMs * 5.0;

            Console::eventStruct Event; //이벤트를 받을 구조체
            while(Console::pollEvent(&Event)){
                if(Event.eventType == E_MOUSE_EVENT){ //만약 마우스 이벤트가 발생하였다면
                    this->patchPlayer(Event.coordinate); //플레어의 위치를 패치한다.
                }else if(Event.eventType == E_KEY_EVENT){ //만약 키보드 이벤트가 발생하였다면
                    if(Event.keyPressed == true && Event.key == PAUSE_KEY){ //만약 정지 키(defined by PAUSE_KEY)가 눌렸다면
                        int todo = this->SCREENpause(); //정지 화면을 출력하고, 반환값을 todo에 저장한다.
                        lastTimeMs = emscripten_get_now();
                        accumulatorMs = 0.0;
                        if(todo == 1){ //만약 todo가 1 이라면(게임 종료)
                            gameStatus = false;
                            break;
                        }
                        else if(todo == 2){ //만약 todo가 2 라면(게임 계속하기)
                            this->printframe->printBlank(); //blank 출력
                            this->printframe->printLogo(); //로고 출력
                            this->printframe->printAlert(1); //알림 출력
                            this->nowAlertcode = 1; //알림 코드 설정
                            this->printframe->printScoreframe(); //점수 프레임 출력
                        }
                        else if(todo == 3) this->init(); //만약 todo가 3이라면(게임 다시시작) -> init()을 통해 배열이나 체력등을 초기화 한 후 진행
                    }
                }
            }
            if(gameStatus == false) break;

            bool updated = false;
            if(accumulatorMs >= tickMs && gameStatus){
                if(this->distance % this->levelCriteria == 0){ //만약 distance가 levelCriteria의 배수라면
                    this->level++; //level을 1 증가시킨다.
                    if(this->level % 3 == 0) this->printframe->SkipFramePer++; //만약 level이 3의 배수면 SkipFramePer을 1 증가시킨다. (체감 속도 증가)
                }
                this->distance++; //distance을 1 증가시킨다.

                gameStatus = this->updateFrame(); //프레임을 업데이트한다.
                this->printFrame(); //프레임을 출력한다.
                updated = true;
                accumulatorMs -= tickMs;
            }
            if(gameStatus == false) break;

            if(updated == false){
                double remainMs = tickMs - accumulatorMs;
                if(remainMs > 1.0) Console::sleep((float)(remainMs / 1000.0));
                else Console::sleep(0.001f);
            }
        }
#else
        promise<Console::eventStruct> p; //p를 받겠다고 약속한다.
        future<Console::eventStruct> coor = p.get_future(); //coor을 통해 미래에 p를 받겠다고 선언한다.
        thread t(Console::waitEvent, &p); //waitEvent를 실행해 p에 받겠다는 약속을 하고 t라는 스레드를 생성한다.
        while (gameStatus) { //gameStatus가 false가 아니면 계속 반복한다.
            Console::eventStruct Event; //이벤트를 받을 구조체
            future_status status = coor.wait_for(std::chrono::milliseconds((int)(this->printframe->interval * 1000))); //미래에 받겠다고 한 coor이 완료가 되었는지 interval초 동안 물어본다.

             if(this->distance % this->levelCriteria == 0){ //만약 distance가 levelCriteria의 배수라면
                this->level++; //level을 1 증가시킨다.
                if(this->level % 3 == 0) this->printframe->SkipFramePer++; //만약 level이 3의 배수면 SkipFramePer을 1 증가시킨다. (체감 속도 증가)
            }
            this->distance++; //distance을 1 증가시킨다.
             
            if (status == future_status::timeout){ //만약 물어본지 1초가 지나 timeout되었다면(시간초과 되었다면)
                gameStatus = this->updateFrame(); //프레임을 업데이트한다.
                this->printFrame(); //프레임을 프린트한다.
                if(gameStatus == false) break; //만약 프레임을 업데이트 할 때 false가 반환이 되었으면 while문을 나간다.
            }
            else if (status == future_status::ready){ //만약 물어봤을때 함수의 반환이 준비가 되었다면
                Event = coor.get(); //미래에 받겠다고 한 정보를 반환받는다.

                if(Event.eventType == E_MOUSE_EVENT){ //만약 마우스 이벤트가 발생하였다면
                    this->patchPlayer(Event.coordinate); //플레어의 위치를 패치한다.
                }else if(Event.eventType == E_KEY_EVENT){ //만약 키보드 이벤트가 발생하였다면
                    if(Event.keyPressed == true && Event.key == PAUSE_KEY){ //만약 정지 키(defined by PAUSE_KEY)가 눌렸다면
                        int todo = this->SCREENpause(); //정지 화면을 출력하고, 반환값을 todo에 저장한다.
                        if(todo == 1){ //만약 todo가 1 이라면(게임 종료)
                            gameStatus = false;
                            break;
                        }
                        else if(todo == 2){ //만약 todo가 2 라면(게임 계속하기)
                            this->printframe->printBlank(); //blank 출력
                            this->printframe->printLogo(); //로고 출력
                            this->printframe->printAlert(1); //알림 출력
                            this->nowAlertcode = 1; //알림 코드 설정
                            this->printframe->printScoreframe(); //점수 프레임 출력
                        }
                        else if(todo == 3) this->init(); //만약 todo가 3이라면(게임 다시시작) -> init()을 통해 배열이나 체력등을 초기화 한 후 진행
                    }
                }

                this->updateFrame(); //프레임을 업데이트한다.
                this->printFrame(); //프레임을 출력한다.
                Console::sleep(this->printframe->interval); //interval초 동안 정지한다.
                break; //while문을 나간다.
            }
        }
        t.join(); //스레드 t를 종료한다.
#endif
        if(gameStatus == false) break;
    }
    return this->SCREENover(); //만약 계속 반복되던 while문이 break되어 종료되면 게임 오버로 간다.
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
        if(this->meteorWarningVisible == true){
            this->printframe->printColorLine(B_WHITE, this->meteorWarningBackColor, this->meteorHorizontal);
        }
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
    this->meteorWarningVisible = false;

    if(this->distance % this->bulletClock == 0){
        if(this->frame[this->printframe->vertical-2][this->PlayerHorizontal].object > 2){
            this->frame[this->printframe->vertical-2][this->PlayerHorizontal].back->object = BULLET;
            this->frame[this->printframe->vertical-2][this->PlayerHorizontal].back->health = H_BULLET;
        }else{
            this->frame[this->printframe->vertical-2][this->PlayerHorizontal].object = BULLET;
            this->frame[this->printframe->vertical-2][this->PlayerHorizontal].health = H_BULLET;
        }
    }

    if(this->distance % this->meteorClock == 0){
        if(this->frame[0][this->meteorHorizontal].object > 2){
            this->frame[0][this->meteorHorizontal].back->object = METEOR;
            this->frame[0][this->meteorHorizontal].back->health = H_METEOR;
        }else{
            this->frame[0][this->meteorHorizontal].object = METEOR;
            this->frame[0][this->meteorHorizontal].health = H_METEOR;
        }
    }else if(this->distance % this->meteorClock == this->meteorClock-41) this->meteorHorizontal = this->PlayerHorizontal;
    else if(this->distance % this->meteorClock >= this->meteorClock-40 && this->distance % this->meteorClock <= this->meteorClock-31){
        this->meteorWarningVisible = true;
        this->meteorWarningBackColor = B_RED;
    }
    else if(this->distance % this->meteorClock >= this->meteorClock-30 && this->distance % this->meteorClock <= this->meteorClock-21){
        this->meteorWarningVisible = true;
        this->meteorWarningBackColor = B_PURPLE;
    }
    else if(this->distance % this->meteorClock >= this->meteorClock-20 && this->distance % this->meteorClock <= this->meteorClock-11){
        this->meteorWarningVisible = true;
        this->meteorWarningBackColor = B_RED;
    }
    else if(this->distance % this->meteorClock >= this->meteorClock-10 && this->distance % this->meteorClock <= this->meteorClock-1){
        this->meteorWarningVisible = true;
        this->meteorWarningBackColor = B_PURPLE;
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
        if(this->nowAlertcode != 1){
            this->printframe->printAlert(1);
            this->nowAlertcode = 1;
        }
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal].object = NONE;
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal].health = H_NONE;
        this->PlayerHorizontal = coor.x - 1 - this->printframe->LeftSpace;
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal].object = PLAYER;
        this->frame[this->printframe->vertical-1][this->PlayerHorizontal].health = this->PlayerHealth;
    }else{
        Console::gotoxy(0, this->printframe->vertical+5);
        if(this->nowAlertcode != 2){
            this->printframe->printAlert(2);
            this->nowAlertcode = 2;
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

    for(int v=this->printframe->vertical-2;v>=0;v--){ //meteor shift
        for(int h=0;h<this->printframe->horizontal;h++){
            if(this->frame[v][h].object == METEOR){ //만약 현재 오브젝트가 meteor이면
                if(v == this->printframe->vertical-2){
                    if(this->frame[v+1][h].object == PLAYER){
                        if(this->PlayerHealth > 1){ //만약 플레이어의 체력이 1보다 크다면
                            this->PlayerHealth--; //플레이어의 체력을 1 감소시킨다.
                            this->frame[v+1][h].health = this->PlayerHealth; //체력 감소를 배열에 반영한다.
                        }else{ //플레이어의 체력이 1 이하라면
                            return false; //게임 오버
                        }
                    }
                    if(this->frame[v][h].back->object != NONE){
                        this->frame[v][h].object = this->frame[v][h].back->object;
                        this->frame[v][h].health = this->frame[v][h].back->health;
                        this->frame[v][h].back->object = NONE;
                        this->frame[v][h].back->object = H_NONE;
                    }else{
                        this->frame[v][h].object = NONE;
                        this->frame[v][h].health = H_NONE;
                    }
                }
                else if(this->frame[v][h].back->object > BULLET){ //만약 현재 오브젝트 뒤에 몬스터가 있으면
                    this->frame[v+1][h].object = METEOR;
                    this->frame[v+1][h].health = H_METEOR;
                    this->frame[v][h].object = this->frame[v][h].back->object;
                    this->frame[v][h].health = this->frame[v][h].back->health;
                    this->frame[v][h].back->object = NONE;
                    this->frame[v][h].back->health = H_NONE;
                }
                else if(this->frame[v+1][h].object > BULLET){ //만약 다음 줄 오브젝트가 몬스터이면
                    this->frame[v+1][h].back->object = this->frame[v+1][h].object; //meteor 뒤에 몬스터 오브젝트를 옮긴다.
                    this->frame[v+1][h].back->health = this->frame[v+1][h].health;
                    this->frame[v+1][h].object = METEOR;
                    this->frame[v+1][h].health = H_METEOR;
                    this->frame[v][h].object = NONE;
                    this->frame[v][h].health = H_NONE;
                }else{
                    this->frame[v+1][h].object = METEOR;
                    this->frame[v+1][h].health = H_METEOR;
                    this->frame[v][h].object = NONE;
                    this->frame[v][h].health = H_NONE;
                }
            }
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

    int MonsterClock = (this->FrameClock - this->printframe->SkipFramePer > 2) ? this->FrameClock - this->printframe->SkipFramePer : 2;
    if(this->distance % MonsterClock == 0){ //만약 클럭이 (FrameClock - levelCriteria)의 배수이면
        for(int v=this->printframe->vertical-2;v>=0;v--){
            for(int h=0;h<this->printframe->horizontal;h++){
                if(v == this->printframe->vertical-2){ //만약 현재 행이 (마지막 행 - 1)의 이라면
                    if(this->frame[v][h].object > BULLET && this->frame[v+1][h].object == PLAYER){ //만약 현재 오브젝트가 몬스터(3 이상) 이고 다음 행에 플레이어가 위치하고 있으면 (플레이어와 몬스터가 충돌할 조건)
                        if(this->PlayerHealth > 1){ //만약 플레이어의 체력이 1보다 크다면
                            this->PlayerHealth--; //플레이어의 체력을 1 감소시킨다.
                            this->frame[v+1][h].health = this->PlayerHealth; //체력 감소를 배열에 반영한다.
                        }else{ //플레이어의 체력이 1 이하라면
                            return false; //게임 오버
                        }
                    }

                    this->frame[v][h].object = NONE;
                    this->frame[v][h].health = H_NONE;
                    continue; //이줄 밑의 코드를 실행하지 않고 바로 다음 반복문을 실행한다.
                }

                if(this->frame[v][h].object == BULLET || this->frame[v][h].object == METEOR){ //만약 현재 오브젝트가 bullet이면
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

void Game::addScore(int target){ //target에 대응되는 점수를 score에 더한다.
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

int Game::SCREENpause(){ //일시정지 화면 출력 함수를 호출 후 키보드와 마우스 입력을 감지하여 알맞는 반환값을 반환한다.
    this->printframe->printPause();
    Console::useEventInput(true);
    while(1){
        Console::eventStruct event;
        Console::getEvent(&event);
        if(event.eventType == E_KEY_EVENT){
            if(event.eventType == E_KEY_EVENT){
                if(event.keyPressed == true && event.key == E_Q_KEY) return 1; //종료하기
                else if(event.keyPressed == true && event.key == E_W_KEY) return 2; //복귀하기
                else if(event.keyPressed == true && event.key == E_E_KEY) return 3; //다시하기
            }
        }
        else if(event.eventType == E_MOUSE_EVENT){
            if(event.Clicked == true && event.ClickKey == E_MOUSE_LEFT){
                //Console::gotoxy(0, 0);
                //printf("%d %d", event.coordinate.x, event.coordinate.y);
                if(event.coordinate.x >= 83 && event.coordinate.x <= 96 && event.coordinate.y>=14 && event.coordinate.y<=16) return 1; //종료하기
                else if(event.coordinate.x >= 83 && event.coordinate.x <= 96 && event.coordinate.y>=18 && event.coordinate.y<=20) return 2; //복귀하기
                else if(event.coordinate.x >= 83 && event.coordinate.x <= 96 && event.coordinate.y>=22 && event.coordinate.y<=24) return 3; //다시하기
            }
        }
    }

    Console::useEventInput(false);
    Console::cls();
    Console::windowSize(this->printframe->consolehorizontal, this->printframe->consolevertical);
    Console::moveWindowCenter();
    Console::cursorVisible(false);
    this->printframe->printLogo();
    this->printframe->printAlert(1);
    this->nowAlertcode = 1;
    this->printframe->printScoreframe();
    Console::useEventInput(true);
}

int Game::SCREENmain(){ //
    this->printframe->printMain();
    Console::useEventInput(true); //메인 화면 출력 함수를 호출 후 키보드와 마우스 입력을 감지하여 알맞는 반환값을 반환한다.
    while(1){
        Console::eventStruct event;
        Console::getEvent(&event);
        if(event.eventType == E_KEY_EVENT){
            if(event.keyPressed == true && event.key == E_Q_KEY) return 1; //시작하기
            else if(event.keyPressed == true && event.key == E_W_KEY) return 2; //종료하기
        }
        else if(event.eventType == E_MOUSE_EVENT){
            if(event.Clicked == true && event.ClickKey == E_MOUSE_LEFT){
                //Console::gotoxy(0, 0);
                //printf("%d %d", event.coordinate.x, event.coordinate.y);
                if(event.coordinate.x >= 83 && event.coordinate.x <= 96 && event.coordinate.y>=35 && event.coordinate.y<=37) return 1; //시작하기
                else if(event.coordinate.x >= 83 && event.coordinate.x <= 96 && event.coordinate.y>=39 && event.coordinate.y<=41) return 2; //종료하기
            }
        }
    }
}

int Game::SCREENover(){ //게임 오버 화면 출력 함수를 호출 후 키보드와 마우스 입력을 감지하여 알맞는 반환값을 반환한다.
    this->printframe->printGameOver(this->score, this->distance, this->level);
    Console::useEventInput(true); //마우스 사용을 선언한다.
    while(1){
        Console::eventStruct event;
        Console::getEvent(&event);
        if(event.eventType == E_KEY_EVENT){
            if(event.keyPressed == true && event.key == E_Q_KEY) return 1; //다시시작
            else if(event.keyPressed == true && event.key == E_W_KEY) return 2; //종료하기
            else if(event.keyPressed == true && event.key == E_E_KEY) return 3; //메인화면
        }
        else if(event.eventType == E_MOUSE_EVENT){
            if(event.Clicked == true && event.ClickKey == E_MOUSE_LEFT){
                //Console::gotoxy(0, 0);
                //printf("%d %d", event.coordinate.x, event.coordinate.y);
                if(event.coordinate.x >= 83 && event.coordinate.x <= 96 && event.coordinate.y>=14 && event.coordinate.y<=16) return 1; //다시시작
                else if(event.coordinate.x >= 83 && event.coordinate.x <= 96 && event.coordinate.y>=18 && event.coordinate.y<=20) return 2; //종료하기
                else if(event.coordinate.x >= 83 && event.coordinate.x <= 96 && event.coordinate.y>=22 && event.coordinate.y<=24) return 3; //메인화면
            }
        }
    }
}

int main(){
    int todo;
    bool KeepWhile = true;
    Game game;
    Console::cursorVisible(false);

    Console::sleep(0.5);
    game.printframe->printIntro(); //인트로 프린트

    todo = game.SCREENmain();
    while(KeepWhile){
        if(todo == 1){
            game.init();
            todo = game.makeClock();
            if(todo == 3) todo = game.SCREENmain();
        }
        else if(todo == 2){
            Console::useEventInput(false);
            Console::cls();
            Console::ErrorExit("게임이 종료되었습니다. [아무키나 누르세요.]");
        }
        else if(todo == 3){
            //튜토리얼 화면
        }else Console::ErrorExit("Error Occured in [main()] with error variable [todo] value");
    }
}
