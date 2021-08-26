#include <stdio.h>
#include <stdlib.h>

enum states{
	State0,
	State1,
	State2,
	State3
};
enum states CurrentState;
int done = 0;
int Test1(){
	int num;
	printf("Enter Number bigger than 0: \n");
	scanf("%d", &num);
	if(num > 0)
		return 1;
	return 0;
}
int Test2(){
	int num2;
	printf("Enter Number bigger than 0: \n");
	scanf("%d", &num2);
	if(num2 > 0)
		return 1;
	return 0;
}
int Test3(){
	int num3;
	printf("Enter Number bigger than 0: \n");
	scanf("%d", &num3);
	if(num3 > 0)
		return 1;
	return 0;
}

int main(void) {

	while(!done){
		switch(CurrentState){
		case State0:
			CurrentState = State1;
			printf("State %d\n", CurrentState);
			break;
		case State1:

			if(Test1())
				CurrentState = State3;
			else
				CurrentState = State2;
			printf("State %d\n", CurrentState);
			break;
		case State2:

			if(Test2())
				CurrentState = State3;
			else
				CurrentState = State0;
			printf("State %d\n", CurrentState);
			break;
		case State3:

			if(!Test3())
				CurrentState = State0;
			printf("State %d\n", CurrentState);
			break;
		}
	}
	return EXIT_SUCCESS;
}
