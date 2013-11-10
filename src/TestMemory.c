/*
 * =====================================================================================
 *
 *       Filename:  TestMemory.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/25/2013 10:16:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jiangwenlong (http://blog.csdn.net/chlaws), jiangwenlong@pipi.cn
 *        Company:  PIPI
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int count = 0;
	int cell = 1024*1024;
	char* maddr = NULL;
	while(1){
		maddr = (char*)malloc(cell);
		if( !maddr ) break;
		int i = 0;
		for(; i < cell/4; ++i){
			//*((int*)(maddr+i)) = i;
			((int*)maddr)[i] = i;
		}
		++count;
		printf("current alloc memory :%dM\n",count);
	}

	return 0;
}
