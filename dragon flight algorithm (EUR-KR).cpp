main(){
	do(��Ʈ�θ� ����Ʈ�Ѵ�.) //printIntro()
	do(����ȭ���� ����Ʈ�Ѵ�.) //Game::SCREENmain()
	
	while(true){
		if(���� ȭ�鿡�� ���õ� �׸� == �����ϱ�){
			do(���� ���� �� �迭�̳� ü�µ��� �ʱⰪ���� �ʱ�ȭ�Ѵ�.) //Game::init()
			while(������ ������� �ʾҴٸ�){ //Game::makeClock()
				co-do(�̺�Ʈ �Է��� �޴´�.) //thread(Console::waitEvent)
				if(���콺�� �������ٸ�){
					do(�÷��̾ ���콺�� x��ǥ�� ���Ͽ� �迭�� ��ġ�Ѵ�.) //Game::patchPlayer()
				} 
				
				if(Ű���尡 ���ȴٸ� and ���� Ű�� W���){
					do(���� ȭ���� ����Ѵ�.) //Game::SCREENpause()
					
					switch(���� ȭ�鿡�� ������ ��){
						case ��������:
							do(������ �����ϰ� ���� ���� ȭ���� ����Ѵ�.) //Game::SCREENover()
							
						case ����ϱ�:
							do(��� �����Ѵ�.)
							
						case �ٽý���:
							do(������ �ʱⰪ���� �ʱ�ȭ�ϰ� ��� �����Ѵ�.) //Game::init()
					}
				}
				
				do(�迭(����, �Ѿ� ��)�� ������Ʈ�Ѵ�.) //Game::updateFrame()
				do(�迭�� ����Ѵ�.) //printFrame()
			}
			do(���� ���� ȭ���� ����Ѵ�.) //Game::SCREENover()
		}
		else if(���� ȭ�鿡�� ���õ� �׸� == �����ϱ�){
			do(Console::ErrorExit()) //���α׷��� �����ϴ� �Լ��� ȣ���Ѵ�. 
		}	
	} 
}
