#include <stdio.h>
#include <unistd.h>

int main(){

	for (int i = 0; i < 256; i += 30){
		for (int j = 0; j < 256; j += 30){
			for (int k = 0; k < 256; k += 60){
				printf("\033[38;2;%d;%d;%dm" "Pretty Pretty colors! %d %d %d\n""\033[0m", i,j,k,i,j,k);
			}
		}
	}
	
	printf("\033[38;2;45;123;190m" "Pretty Pretty colors!\n");

}