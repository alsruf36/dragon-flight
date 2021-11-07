#include<Windows.h>
#include<stdio.h>
 
int main(int argc, char *argv[]){
    INPUT_RECORD rc;
    DWORD        dw;
    int mouse_XY[2];
    COORD pos={0,0};
    
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
    int t=0;
    while(++t){
        
        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &rc, 1, &dw); //핸들정보 
        mouse_XY[0] = rc.Event.MouseEvent.dwMousePosition.X; //X좌표 
        mouse_XY[1] = rc.Event.MouseEvent.dwMousePosition.Y; //Y좌표 

        FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', 100 , pos, &dw); //화면 지우기         
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos); //콘솔좌표이동 
        printf("%d, %d, %d\n", mouse_XY[0], mouse_XY[1], t);       
        
    }
    return 0;
}
