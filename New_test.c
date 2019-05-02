#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<error.h>
#include<stdlib.h>
#include<termios.h>

#define DEV_NAME  "/dev/ttyUSB0"


static unsigned char stroage_addr[50];
static int sto_add_len;
static unsigned char stroage_len[30];
static double stroage_cofee[10];
static unsigned char stroage_name[10][20];
static int sto_reg_len;
static int stroage_cofee_len;
static int sto_name_len;
static length_cnt;
static ana_flag;


static unsigned char fixed_data[2];
static unsigned char stroage_data[5];
static int name_i[5]={1,2,3,4,5};
static unsigned char temp_name[10];
static unsigned char Dev_info[8];
static unsigned char recv_info[8];


static int SetUart_opt(int fd,int nSpeed,int nBits,char nEvent,int nStop);
static void setup_back();
void anasyses_name_info(char*str);
void anasyses_fixed_info(char *str);
void anasyses_cout(char *str);
void anasyses_addr(char *str);
void anasyses_coeff(char *str);
void anasyses_len(char *str);
static void anasyses_recvdata();
unsigned short ModBusCRC (unsigned char *ptr,unsigned char size);
void Set_BuffCRC(unsigned char *str);

static int SetUart_opt(int fd,int nSpeed,int nBits,char nEvent,int nStop)
{
	struct termios newuat,olduat;  
	if(tcgetattr(fd,&olduat)!=0)
	{
		perror("tcgetattr");
		printf("get uart info error\n");
		return -1;
	}
	bzero(&newuat,sizeof(struct termios));
	newuat.c_cflag|=CREAD|CLOCAL;
	newuat.c_cflag&=~(CSIZE);
	switch(nBits)
	{
		case 7:
				newuat.c_cflag|=CS7;
				break;
		case 8:
				newuat.c_cflag|=CS8;
				break;
		default:
				return -1;
		
	}
	switch( nEvent ) 
	{
	case 'O':
		newuat.c_cflag |= PARENB;
		newuat.c_cflag |= PARODD;
		newuat.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'E': 
		newuat.c_iflag |= (INPCK | ISTRIP);
		newuat.c_cflag |= PARENB;
		newuat.c_cflag &= ~PARODD;
		break;
		case 'N':  
		newuat.c_cflag &= ~PARENB;
		break;
	}
	switch( nSpeed )
	{
		case 2400:
			cfsetispeed(&newuat, B2400);       
			cfsetospeed(&newuat, B2400);
			break;
		case 4800:
			cfsetispeed(&newuat, B4800);
			cfsetospeed(&newuat, B4800);
			break;
		case 9600:
			cfsetispeed(&newuat, B9600);
			cfsetospeed(&newuat, B9600);
			break;
		case 115200:
			cfsetispeed(&newuat, B115200);
			cfsetospeed(&newuat, B115200);
			break;
		case 460800:
			cfsetispeed(&newuat, B460800);
			cfsetospeed(&newuat, B460800);
			break;
		default:
			cfsetispeed(&newuat, B9600);
			cfsetospeed(&newuat, B9600);
			break;
	}
	if( nStop == 1 )
		newuat.c_cflag &=  ~CSTOPB;
	else if ( nStop == 2 )
		newuat.c_cflag |=  CSTOPB;
		newuat.c_cc[VTIME]  = 100;
		newuat.c_cc[VMIN] = 0;
		tcflush(fd,TCIFLUSH);
	if((tcsetattr(fd,TCSANOW,&newuat))!=0)
	{
		perror("com set error");
		return -1;
	}
	return 0;
}

unsigned short ModBusCRC (unsigned char *ptr,unsigned char size)

{

unsigned short a,b,tmp,CRC16,V;

CRC16=0xFFFF; 

for (a=0;a<size;a++)

{

   CRC16=*ptr^(CRC16);

   for (b=0;b<8;b++)  

   {

     tmp=CRC16 & 0x0001;

     CRC16 =CRC16 >>1; 

   if (tmp)
   {
       CRC16=CRC16 ^ 0xa001; 
    }
	}
 *ptr++;

} 

  V = ((CRC16 & 0x00FF) << 8) | ((CRC16 & 0xFF00) >> 8) ;
   return V;

}



void Set_BuffCRC(unsigned char *str)
{
	Dev_info[6]=ModBusCRC(Dev_info,6);
	//printf("%x\n",ModBusCRC(Dev_info,6));
	unsigned char x1=((ModBusCRC(Dev_info,6))>>8);
	unsigned char x2=ModBusCRC(Dev_info,6);
	//printf("x1=%x,x2=%x\n",x1,x2);
	Dev_info[6]=x1;
	Dev_info[7]=x2;
}


void anasyses_fixed_info(char *str)
{
		unsigned char test=0x00;
		int i=0;
		int de_addr_len=strlen("device_address=");
		int fun_len=strlen("function_code=");
		char *re=NULL;
		int  num[3]={1,2,3};
		int tmp=0;
		int count=0;
		while(*str++)
		{
			if(re=strstr(str,"device_address="))
			{
					re+=de_addr_len;
					#if 0
					re-=1;
					for(tmp=1;tmp<4;tmp++)
					{
							if(*(re+=tmp) == ' ')
							{
								count=tmp;
								re-=count;
							}
							else
							{
								re-=count;
							}
					}
					#endif
					strncpy(stroage_data,re,3);
					i=atoi(stroage_data);
					if(i>(0xA))
					{
						i+=6;
					}
					test|=i;
					fixed_data[0]=test;
					memset(stroage_data,0,sizeof(stroage_data));
					test=0x00;
					i=0;
					str=NULL;
					str=re;
				//	printf("=%x\n",fixed_data[0] );
			}
			re=NULL;
			if(re=strstr(str,"function_code="))
			{
				re+=fun_len;
				strncpy(stroage_data,re,1);
				i=atoi(stroage_data);
				test|=i;
				fixed_data[1]=test;
				memset(stroage_data,0,sizeof(stroage_data));
			}
		}
}

void anasyses_name_info(char *str)
{
	char *a=NULL;
	char *b=NULL;
	char *c=NULL;
	int i=0;
	int len=0;
	while(*str++)
	{
		if((*str)==(':')  && (a=strchr(str,':')))
		{
			for(i=0;i<5;i++)
			{
				b=a;
				c=a;
				if(*(a-=(i+1))=='\n')
				{
						len=i;
				}
				a=NULL;
				a=b;
			}
			//printf("len=%d\n",len );
			c-=len;
			strncpy(temp_name,c,len);
			strcpy(stroage_name[sto_name_len++],temp_name);
			//printf("%s\n",stroage_name[0]);
		//	stroage_name[sto_name_len]=temp_name[0];
			//printf("%s\n",temp_name );
			memset(temp_name,0,sizeof(temp_name));
		}
		len=0;
		c=NULL;
	}
}

void anasyses_addr(char *str)
{
 	unsigned test=0x00;
 	int len=strlen("addr=");
 	int i=0;
 	char *a=NULL;
 	while(*str++)
 	{
 		if(a=strstr(str,"addr="))
 		{
 			a+=len;
 			strncpy(stroage_data,a,2);
 			i=atoi(stroage_data);
 			if(i>(0xA))
 			{
 				i+=6;
 			}
 			test|=i;
 			stroage_addr[sto_add_len]=test;
 		//printf("%x ",test );
 			a+=2;
 			i=0;
 			test=0x00;
 			memset(stroage_data,0,sizeof(stroage_data));
 			strncpy(stroage_data,a,2);
 		//	unsigned char mdk[10];
 			//sprintf(mdk,"%x",stroage_data);
 			//printf("%x ",mdk);
 			#if 1
 			i=atoi(stroage_data);
 		//	printf("i=%d ", i );
 			test|=i;
 			stroage_addr[++sto_add_len]=test;
 			sto_add_len+=1;
 			//printf("%x\n",test );
 			#endif
 			i=0;
 			test=0x00;
 			memset(stroage_data,0,sizeof(stroage_data));
 			str=NULL;
 			str=a;
 		}
 	}

}

void anasyses_len(char *str)
{

	int len=strlen("length=");
	char *a=NULL;
	int i=0;
	unsigned char test=0x00;
	while(*str++)
	{
		if(a=strstr(str,"length="))
		{
				length_cnt++;
				stroage_len[sto_reg_len]=test;
				a+=len;
				strncpy(stroage_data,a,1);
				i=atoi(stroage_data);
				test|=i;
				//printf("test=%x\n",test );
				stroage_len[++sto_reg_len]=test;
				sto_reg_len+=1;
			 
				memset(stroage_data,0,sizeof(stroage_data));
				i=0;
				str=NULL;
				test=0x00;
				str=a;
		}		
	}
}

void anasyses_coeff(char *str)
{
	int len=strlen("coeff=");
	char *a=NULL;
	double i=0;
	while(*str++)
	{
		if(a=strstr(str,"coeff"))
		{
			a+=len;
			strncpy(stroage_data,a,5);
			i=strtod(stroage_data,NULL);
			stroage_cofee[stroage_cofee_len]=i;
		//	printf("%lf  ",stroage_cofee[stroage_cofee_len] );
			stroage_cofee_len++;
		//	printf("%.5f\n",i );
			memset(stroage_data,0,sizeof(stroage_data));
			i=0;
			str=NULL;
			str=a;
		}
	}
}

static void setup_back()
{
		 static int ana_addr_cnt=0;
		  static int ana_len_cnt=0;
		Dev_info[0]=fixed_data[0];
		Dev_info[1]=fixed_data[1];
	    Dev_info[2]=stroage_addr[ana_addr_cnt];
		Dev_info[3]=stroage_addr[++ana_addr_cnt];
		ana_addr_cnt++;
		Dev_info[4]=stroage_len[ana_len_cnt];
		Dev_info[5]=stroage_len[++ana_len_cnt];
		ana_len_cnt++;
		Set_BuffCRC(Dev_info);
		int i=0;
		for(i=0;i<8;i++)
		{
			printf("%02x ",Dev_info[i] );
		}
		printf("\n");
}

static void anasyses_recvdata(char *str)
{
		unsigned int a=0xFFFF;
		a&=~(0xFFFF);
		a|=str[3];
		a=a<<8;
		a|=str[4];
		double b=a;
		
		printf("%s=%.2f\n",stroage_name[ana_flag++],(b*(stroage_cofee[ana_flag])));
}

int main(int argc, char const **argv)
{
	if(argc!=2)
	{
		printf("./a.out + configure file\n");
		return 0;
	}
//while(1)
//{	
	FILE *fp;
	char *as=NULL;
	if((fp=fopen(argv[1],"r+"))==NULL)
	{
		perror("fopen");
		printf("open file failure\n");
		return 0;
	}
	fseek(fp,0,SEEK_END);
	int file_size=ftell(fp);
//	printf("%d\n",file_size );
	rewind(fp);
	as=(char *)malloc(sizeof(char)*file_size);
	if(as==NULL)
	{
		printf("malloc memory failure\n");
		return 0;
	}
	fread(as,1,file_size,fp);

	#if 1
	int fd=0;
	fd=open(DEV_NAME,O_RDWR | O_NOCTTY);
	if(fd<0)
	{
		perror("open");
		printf("open uart device error\n");
		exit(-1);
	}

	SetUart_opt(fd,9600,8,'N',1);
	#endif
	anasyses_fixed_info(as);
	//printf("%x\n",fixed_data[0] );
	//printf("%x\n",fixed_data[1] );
	anasyses_name_info(as);
	anasyses_addr(as);
	int k=0;
	//printf("\n");
	anasyses_len(as);
	anasyses_coeff(as);
	int ana_cnt=0;
	for(ana_cnt=0;ana_cnt<length_cnt;ana_cnt++)
	{
		setup_back();
		//TODO read write fd
		int wir_len=0,recv_len=0;
		wir_len=write(fd,Dev_info,sizeof(Dev_info));
		if(wir_len<0)
		{
			perror("write");
			printf("write faile failure\n");
			return 0;
		}
		sleep(1);
		recv_len=read(fd,recv_info,wir_len);
		if(recv_len<0)
		{
			perror("read");
			printf("read file failure\n");
			return 0;
		}
		anasyses_recvdata(Dev_info);
		memset(Dev_info,0,sizeof(Dev_info));
		memset(recv_info,0,sizeof(recv_info));

	}
	for(k=0;k<10;k++)
	{
	//	printf("%x ",stroage_len[k]);
	}
	//printf("\n");
//	Set_BuffCRC(Dev_info);
	int j=0;
	for(j=0;j<8;j++)
	{
	//	printf("%02x ",Dev_info[j] );
	}
	//printf("\n");
	close(fd);
	fclose(fp);
	free(as);
	as=NULL;
//}
	return 0;
}


