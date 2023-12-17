
/***********************************************************************************************
created: 		2023-12-17

author:			chensong

purpose:		play pcm 

************************************************************************************************/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csdl2_play_aac.h"
#include "csdl2_play_pcm.h"




int main(int argc,  char * argv[]) 
{
	
	//chen::test_sdl2_play_aac("../../data/input.aac");
	chen::test_sdl2_paly_pcm("../../data/input.pcm");
	return 0;
}