main(){
	do(인트로를 프린트한다.) //printIntro()
	do(메인화면을 프린트한다.) //Game::SCREENmain()
	
	while(true){
		if(메인 화면에서 선택된 항목 == 시작하기){
			do(게임 시작 전 배열이나 체력등을 초기값으로 초기화한다.) //Game::init()
			while(게임이 종료되지 않았다면){ //Game::makeClock()
				co-do(이벤트 입력을 받는다.) //thread(Console::waitEvent)
				if(마우스가 움직였다면){
					do(플레이어를 마우스의 x좌표에 대하여 배열에 패치한다.) //Game::patchPlayer()
				} 
				
				if(키보드가 눌렸다면 and 눌린 키가 W라면){
					do(정지 화면을 출력한다.) //Game::SCREENpause()
					
					switch(정지 화면에서 선택한 값){
						case 게임종료:
							do(게임을 종료하고 게임 오버 화면을 출력한다.) //Game::SCREENover()
							
						case 계속하기:
							do(계속 진행한다.)
							
						case 다시시작:
							do(게임을 초기값으로 초기화하고 계속 진행한다.) //Game::init()
					}
				}
				
				do(배열(몬스터, 총알 등)을 업데이트한다.) //Game::updateFrame()
				do(배열을 출력한다.) //printFrame()
			}
			do(게임 오버 화면을 출력한다.) //Game::SCREENover()
		}
		else if(메인 화면에서 선택된 항목 == 종료하기){
			do(Console::ErrorExit()) //프로그램을 종료하는 함수를 호출한다. 
		}	
	} 
}
