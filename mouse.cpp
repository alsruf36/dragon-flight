#include <windows.h>
#include <stdio.h>

INPUT_RECORD rec;
DWORD dwNOER;
HANDLE CIN = 0;

void click(int *xx, int *yy, int *lr){
        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &rec, 1, &dwNOER); // 콘솔창 입력을 받아들임.
        if (rec.EventType == MOUSE_EVENT){// 마우스 이벤트일 경우

            if (rec.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED){ // 좌측 버튼이 클릭되었을 경우
                int mouse_x = rec.Event.MouseEvent.dwMousePosition.X; // X값 받아옴
                int mouse_y = rec.Event.MouseEvent.dwMousePosition.Y; // Y값 받아옴

                *xx=mouse_x; //x값을 넘김
                *yy=mouse_y; //y값을 넘김
                *lr=1; //마우스 클릭상태를 넘김


            }else if(rec.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED){ // 우측 버튼이 클릭되었을 경우
                int mouse_x = rec.Event.MouseEvent.dwMousePosition.X; // X값 받아옴
                int mouse_y = rec.Event.MouseEvent.dwMousePosition.Y; // Y값 받아옴

                *xx=mouse_x; //x값을 넘김
                *yy=mouse_y; //y값을 넘김
                *lr=2; //마우스 클릭상태를 넘김
            }
        }
    
}

int main(){
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT); 
    //마우스 입력모드로 바꿈

    int xx,yy,lr;
    while (1){

        click(&xx, &yy, &lr);
        printf("%2d %2d %2d\n",xx,yy,lr);

    }

}

//https://m.blog.naver.com/PostView.naver?isHttpsRedirect=true&blogId=soohan530&logNo=221017085392