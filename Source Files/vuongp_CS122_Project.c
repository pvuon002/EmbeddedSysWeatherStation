//HEADER FILES OF BMP085 AND TWIMASTER ARE COPYRIGHTED BY (c) Davide Gironi, 2012
//DOWNLOADED FROM http://davidegironi.blogspot.com/2012/10/avr-atmega-bmp085-pressure-sensor.html#.UqOyifRDs-R

#include <avr/io.h>
#include <lcd.h>
#include <scheduler.h>
#include <keypad.h>
#include <stdio.h>
#include <util/twi.h>
#include "twimaster.c"
#include "bmp085.c"

unsigned char timearr[8]={'0','0',':','0','0',':','0','0'}; //array for time
unsigned char tarr[24]= {'T','e','m','p',':',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}; //array for temperature
unsigned char altarr[24]= {'A','l','t',':',' ',' ',' ','m',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}; //array for altitude
volatile unsigned char key='\0'; //key press
unsigned char hr,h1,h2, m1, m2, s1, s2,cnt= 0; //h1 h2 : m1 m2 : s1 s2
unsigned char storetime[4]; // stores time initially inputted by keypad
unsigned char kcnt, clkrdy=0; // clkrdy is a flag to start time and kcnt is value of number of key presses
unsigned char bp,altim=0; //bp is button press of 2 buttons, altim is used to jump from temp display to altitude
unsigned char aflg,tflg,mflg=0; // aflg stops display of time for altimeter SM. tflg begins time SM
double temp=0; //temporary for temperature
double temp2 = 0; //temporary for altitude


//HELPER FUNCTION TO EXTRACT EACH DIGIT FROM AN INT
// SOURCE: http://stackoverflow.com/a/9407401
int digitAtPos(int number, int pos)
{
	return ( number / (int)pow(10.0, pos-1) ) % 10;
}

enum BStates {BPress,BRelease};
int ButtonPress(int state){
	bp= ((~PINC)&0xF0);
	switch (state){ //actions
		case BPress:
		if(bp){
			if (bp==0x10){
				tflg=1;
				aflg=0;
				mflg=0;
				altim=0;
			}
			else if(bp==0x20){
				aflg=1;
				tflg=0;
				mflg=0;
				altim++;
				if(altim>2)
				altim=1;
			}
			else if(bp==0x40){
				aflg=0;
				tflg=0;
				mflg=1;
				altim=0;
			}
			state= BRelease;
		}
		else
		state= BPress;
		break;
		case BRelease:
		if(!bp)
		state= BPress;
		else
		state= BRelease;
		break;
	} //end actions
	return state;
}

enum KStates {WaitP,WaitR,Fin,F};
int keys(int state){
	switch (state){ //transitions
		case WaitP:
		if(key)
		state=WaitR;
		else if(kcnt>3){
			kcnt=0;
			state= Fin;
		}
		else
		state= WaitP;
		break;
		
		case WaitR:
		if(!key)
		{
			kcnt++;
			state= WaitP;
		}
		else
		state= WaitR;
		break;
		
		case Fin:
		state= F;
		break;
		case F:
		state= F;
		break;
	}// end of transitions
	
	switch (state) { //actions
		case WaitR:
		storetime[kcnt]= key;
		LCD_Cursor(kcnt+1);
		LCD_WriteData(key);
		break;
		case Fin:
		h1= storetime[0]-'0'; //converts ascii to char
		h2= storetime[1]-'0';
		m1= storetime[2]-'0';
		m2= storetime[3]-'0';
		timearr[0]=h1+'0';
		timearr[1]=h2+'0';
		timearr[3]=m1+'0';
		timearr[4]=m2+'0';
		
		clkrdy=1;
		LCD_ClearScreen();
		break;
	} // end actions
	
	return state;
}

enum TStates {ClkW,Init};
int settime(int state){
	
	switch(state){ // transitions
		case ClkW:
		if(clkrdy){
			tflg=1;
			LCD_DisplayString(1,timearr);
			state= Init;
		}
		else
		{
			state= ClkW;
		}
		break;
		case Init:
		//if(aflg)
		//	state= Pause;
		//else
		state=Init;
		break;
		//case Pause:
		//if(tflg)
		//	state= Init;
		//else
		//	state= Pause;
	}
	switch(state){
		case Init:
		//LCD_ClearScreen();
		
		cnt++;
		if(cnt>2){
			cnt=0;
			s2++;
			if(s2>9){
				s1++;
				s2=0;
				
			}
			if(s1>5){
				m2++;
				s1=0;
				
			}
			if(m2>9){
				m1++;
				m2=0;
				
			}
			if(m1>5){
				h2++;
				m1=0;
			}
			
			if(h2>9 && h1==0){
				h1++;
				h2=0;
			}
			
			if(h2>2 && h1==1){
				h1=0;
				h2=1;
			}
			
			timearr[0]= h1+'0';
			timearr[1]= h2+'0';
			timearr[3]= m1+'0';
			timearr[4]= m2+'0';
			timearr[6]= s1+'0';
			timearr[7]= s2+'0';
			//LCD_ClearScreen();
			if(tflg)
			LCD_DisplayString(1,timearr);
		}
		break;
	} // end actions
	return state;
}

enum AStates{AltW,A1,A2};
int Altimeter (int state){
	switch(state){ //transitions
		case AltW:
		if(altim)
		state=A1;
		else
		state= AltW;
		break;
		case A1:
		if(altim==2)
		state= A2;
		else if(altim==0)
		state= AltW;
		else
		state= A1;
		break;
		case A2:
		if(altim==0)
		state= AltW;
		else if(altim==1)
		state=A1;
		else
		state= A2;
		break;
	} //end of transitions
	switch(state){ //actions
		case AltW:
		
		break;
		case A1:
		temp= bmp085_gettemperature();
		//convert to Fahrenheit
		temp= temp*9;
		temp= temp/5;
		temp= temp+32;
		temp= (int)temp;
		tarr[5]= digitAtPos(temp,3)+'0';
		tarr[6]= digitAtPos(temp,2)+'0';
		tarr[7]= digitAtPos(temp,1)+'0';
		//LCD_ClearScreen();
		LCD_DisplayString(1,tarr);
		break;
		case A2:
		temp2= (int)bmp085_getaltitude();
		altarr[4]= digitAtPos(temp2,3)+'0';
		altarr[5]= digitAtPos(temp2,2)+'0';
		altarr[6]= digitAtPos(temp2,1)+'0';
		//LCD_ClearScreen();
		LCD_DisplayString(1,altarr);
		break;
	} //end actions
	return state;
}
int x=0;
unsigned char msg[50];
unsigned char prev='\0';
unsigned char prevc='\0';
int c=1;
int cnt0,cnt1,cnt2,cnt3,cnt4,cnt5,cnt6,cnt7,cnt8,cnt9=0;
unsigned char one[4]={'1','1','1','1'};
unsigned char zero[4]={'0','0','0','0'};
unsigned char two[4]={'a','b','c','2'};
unsigned char three[4]={'d','e','f','3'};
unsigned char four[4]={'g','h','i','4'};
unsigned char five[4]={'j','k','l','5'};
unsigned char six[4]={'m','n','o','6'};
unsigned char sev[5]={'p','q','r','s','7'};
unsigned char eight[4]={'t','u','v','8'};
unsigned char nine[5]={'w','x','y','z','9'};

enum Mstates{MWait,press,release,space,show_msg};

int Msgstate (int state)
{
	switch (state){ //Transitions
		case MWait:
		if(mflg){
			LCD_ClearScreen();
			state= press;
		}
		else
		state= MWait;
		break;
		case press:
		if(!mflg)
		state=MWait;
		else if(key== 'C')
		state=show_msg;
		else if(key=='\0')
		state= press;
		else if(key=='D' || (key!=prev && prevc!='\0'))
		state= space;
		else
		{
			//CONDITIONS ON THE FLY
			
			if ( key == '0')
			{
				prevc= zero[cnt0];
				cnt0++;
				if(cnt0==4)
				cnt0=0;
			}
			else if( key== '1')
			{
				prevc=one[cnt1];
				cnt1++;
				if(cnt1==4)
				cnt1=0;
			}
			else if (key=='2')
			{
				prevc= two[cnt2];
				cnt2++;
				if(cnt2==4)
				cnt2=0;
			}
			else if (key== '3')
			{
				prevc= three[cnt3];
				cnt3++;
				if(cnt3==4)
				cnt3=0;
			}
			else if (key== '4')
			{
				prevc= four[cnt4];
				cnt4++;
				if(cnt4==4)
				cnt4=0;
			}
			else if (key== '5')
			{
				prevc= five[cnt5];
				cnt5++;
				if(cnt5==4)
				cnt5=0;
			}
			else if (key== '6')
			{
				prevc= six[cnt6];
				cnt6++;
				if(cnt6==4)
				cnt6=0;
			}
			else if (key== '7')
			{
				prevc= sev[cnt7];
				cnt7++;
				if(cnt7==5)
				cnt7=0;
			}
			else if (key== '8')
			{
				prevc= eight[cnt8];
				cnt8++;
				if(cnt8==4)
				cnt8=0;
			}
			else if (key== '9')
			{
				prevc= nine[cnt9];
				cnt9++;
				if(cnt9==5)
				cnt9=0;
			}
			prev= key;
			LCD_Cursor(c);
			LCD_WriteData(prevc);
			
			state= release;
		}
		break;
		
		case release:
		if(!mflg)
		state= MWait;
		else if(key== 'C')
		state=show_msg;
		else if(key!='\0')
		state= release;
		else if (key=='D')
		state= space;
		else
		state= press;
		break;
		case space:
		//CONDITIONS ON THE FLY
		if(!mflg)
		state= MWait;
		else if ( key == '0')
		{
			prevc= zero[cnt0];
			cnt0++;
			if(cnt0==4)
			cnt0=0;
		}
		else if( key== '1')
		{
			prevc=one[cnt1];
			cnt1++;
			if(cnt1==4)
			cnt1=0;
		}
		else if (key=='2')
		{
			prevc= two[cnt2];
			cnt2++;
			if(cnt2==4)
			cnt2=0;
		}
		else if (key== '3')
		{
			prevc= three[cnt3];
			cnt3++;
			if(cnt3==4)
			cnt3=0;
		}
		else if (key== '4')
		{
			prevc= four[cnt4];
			cnt4++;
			if(cnt4==4)
			cnt4=0;
		}
		else if (key== '5')
		{
			prevc= five[cnt5];
			cnt5++;
			if(cnt5==4)
			cnt5=0;
		}
		else if (key== '6')
		{
			prevc= six[cnt6];
			cnt6++;
			if(cnt6==4)
			cnt6=0;
		}
		else if (key== '7')
		{
			prevc= sev[cnt7];
			cnt7++;
			if(cnt7==5)
			cnt7=0;
		}
		else if (key== '8')
		{
			prevc= eight[cnt8];
			cnt8++;
			if(cnt8==4)
			cnt8=0;
		}
		else if (key== '9')
		{
			prevc= nine[cnt9];
			cnt9++;
			if(cnt9==5)
			cnt9=0;
		}
		prev= key;
		//LCD_Cursor(c);
		//LCD_WriteData(prevc);
		
		state= release;
		break;
		case show_msg:
		if(!mflg)
		state= MWait;
		if(~PINC&0x40){
			LCD_ClearScreen();
			state= press;
		}
		else
		state= show_msg;
		break;
	} // end of transitions
	
	switch(state){ //Actions
		case space:
		if(key== 'D')
		{
			msg[x]=prevc;
			x++;
			msg[x]=' ';
			x++;
		}
		
		c++;
		
		break;
		case show_msg:
		//LCD_DisplayString(1,msg);
		cnt0,cnt1,cnt2,cnt3,cnt4,cnt5,cnt6,cnt7,cnt8,cnt9=0;
		c=0;
		x=0;
		break;
	} // end of Actions
	
	return state;
}


int main(void)
{
	DDRA= 0xF0;	PORTA= 0x0F; //KEYPAD
	DDRB=0xFF; PORTB= 0x00; // PIN 4 and 5 for LCD
	DDRD= 0xFF; PORTD= 0x00; // LCD
	DDRC= 0x00; PORTC= 0xFF; //sensor
	
	tasksNum = 5; // declare number of tasks
	task tsks[5]; // initialize the task array
	tasks = tsks; // set the task array
	
	unsigned char i=0; // task counter
	tasks[i].state = WaitP;
	tasks[i].period = 30;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &keys;
	i++;
	
	s1=5;
	s2=0;
	timearr[6]=s1+'0';
	timearr[7]=s2+'0';
	
	tasks[i].state = ClkW;
	tasks[i].period = 30;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &settime;
	i++;
	
	tasks[i].state = BPress;
	tasks[i].period = 20;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &ButtonPress;
	i++;
	tasks[i].state = AltW;
	tasks[i].period = 20;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &Altimeter;
	i++;
	tasks[i].state = MWait;
	tasks[i].period = 20;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &Msgstate;
	
	LCD_init();
	bmp085_init();
	TimerSet(10); // value set should be GCD of all tasks
	TimerOn();
	
	while(1)
	{
		key= GetKeypadKey();
	}
}