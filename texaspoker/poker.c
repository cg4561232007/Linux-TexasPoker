#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock.h>
#include<time.h>
/*-------------GAME INFO-----------------*/
#define max(a,b)    (((a) > (b)) ? (a) : (b))
typedef unsigned int u_int;
typedef u_int SOCKET;
typedef enum{ SPADES, HEARTS, CLUBS, DIAMONDS } COLOR;
typedef enum{ HIGH, MID, LOW }P_NUM;//��������֮������Ĵ�С
typedef struct poker
{
	int number;
	COLOR color;
}*POKER, MPOKER;
struct info
{
	int pid;
	int jetton;
	int money;
	int bet;
	char *action;
};
typedef enum{
	HIGHCARD, PAIRS, SAMECOLOR, DEFAULT,
	TPAIRS, SANTIAO, SHUNZI, TONGHUA, HULU, SITIAO, KING,
	LKSHUNZI, LKTONGHUA, P_LKSHUNZI, P_LKTONGHUA, SPECIAL
} HANDCARDS;	 //LKSHUNZI�ǲ�һ�ž����˳��,SPECIAL��ʾ���ܻ������ֻ���ͣ���˫�Ժ�˳�ӡ�
/*----------------ȫ�ֱ���---------------*/
HANDCARDS handcards, flophand, turnhand, riverhand;
P_NUM paimian;
MPOKER mhandpk[2], mfpok[3], mtpok, mrpok;
POKER handpk, fpok, tpok, rpok;	 //XIUGAILE
int ROUND = 0;
int game_rd = 0;//ÿ�����500��
int hold_rd, flop_rd, turn_rd, river_rd;
/*---------------------------------------*/
/*-----------------socket----------------*/
char SendBuff[1024] = "\0";
char ReceiveBuff[1024] = "\0";
int SendLen = 0;
SOCKET socket_client;
/*-------------------��������------------------*/
void myaction(struct info[], int, int, int);
char* left(char*, char*, int);
char* mid(char*, char*, int, int);
HANDCARDS checkHold(char *, char *);
HANDCARDS checkFlop(char *, char *, char*);
HANDCARDS checkTurn(char *);
HANDCARDS checkRiver(char *);
HANDCARDS judge(POKER, POKER, POKER, POKER);
int onMessageCheck(char message[])
{
	/*--�����������������Ϣ�ַ������ж���Ϣ����--*/
	const char *delim = "\n";
	char *token[20];
	token[0] = strtok(message, delim);
	printf("�ָ��ַ���:%s\n", token[0]);
	int itok, myi;
	for (itok = 0; itok < 20; itok++)
	{
		token[itok + 1] = strtok(NULL, delim);
		printf("��Ϣ����:%d %s\n", itok + 1, token[itok + 1]);
	}
	for (myi = 0; myi < 20; myi++)
	{
		if (token[myi] == NULL) break;
		if (strcmp(token[myi], "seat/ ") == 0)
		{
			printf("message seat\n");
			game_rd += 1;
			hold_rd = 0;
			flop_rd = 0;
			turn_rd = 0;
			river_rd = 0;
		}
		if (strcmp(token[myi], "game-over/ ") == 0)
		{
			printf("message gameover\n");
			printf("����");
			return 0;
		}

		if (strcmp(token[myi], "blind/ ") == 0)
		{
			printf("message blind\n");
			printf("äע");
			return 0;
		}
		if (strcmp(token[myi], "inquire/ ") == 0)
		{
			printf("message inquire\n");
			return 0;
			switch (ROUND)
			{
			case 1:	hold_rd += 1; break;
			case 2: flop_rd += 1; break;
			case 3:	turn_rd += 1; break;
			case 4: river_rd += 1; break;
			default:
				break;
			}
			//�����Ϣ
			int total_pot;
			char head[10] = "\0";
			char *info[8][6];
			int i = 1, j = 1;
			printf("switch round inquire");
			while (token[i] && atoi(left(head, token[i], 1)))
			{
				char *mes = token[i];
				printf("mes:%s\n", mes);
				info[i - 1][0] = strtok(mes, " ");
				printf("j=0,info[%d][0]=%s\n", i - 1, info[i - 1][0]);
				while ((info[i - 1][j] = strtok(NULL, " ")) != NULL)
				{
					printf("j=%d,info[%d][%d]=%s\n", j, i - 1, j, info[i - 1][j]);
					j++;
				}
				j = 1;
				i++;
			}
			int restp = i - 1;
			//����ṹ������д��������
			struct info maninfo[8];	//ֱ�����ýṹ������Գ�Ա��ֵӦ�ÿ���
			int infosize;
			int k;
			for (k = 0; k < restp; k++)
			{
				maninfo[k].pid = atoi(info[k][0]);
				maninfo[k].jetton = atoi(info[k][1]);
				maninfo[k].money = atoi(info[k][2]);
				maninfo[k].bet = atoi(info[k][3]);
				strcpy(maninfo[k].action, info[k][4]);
			}
			//�õ�total pot
			char *total_msg = token[i];
			printf("totalmsg=%s\n", total_msg);
			if (strcmp(left(head, total_msg, 5), "total") == 0)
			{
				char result[20] = "wocaonima";
				char *p = mid(result, total_msg, 10, 12);
				total_pot = atoi(p);
			}
			printf("totalpot:%d\n", total_pot);
			printf("inquire");
			myaction(maninfo, total_pot, ROUND, infosize);
			//strcpy(SendBuff, "call");
			//SendLen = send(socket_client, SendBuff, 100, 0);
			return 0;
		}

		if (strcmp(token[myi], "hold/ ") == 0)
		{
			printf("message hold\n");
			ROUND = 1;
			handcards = checkHold(token[1], token[2]);
			return 0;
		}
		if (strcmp(token[myi], "flop/ ") == 0)
		{
			printf("message flop\n");
			ROUND = 2;
			flophand = checkFlop(token[1], token[2], token[3]);
			return 0;
		}
		if (strcmp(token[myi], "turn/ ") == 0)
		{
			printf("message turn\n");
			ROUND = 3;
			turnhand = checkTurn(token[1]);
			return 0;
		}
		if (strcmp(token[myi], "river/ ") == 0)
		{
			printf("message river\n");
			ROUND = 4;
			riverhand = checkRiver(token[1]);
			return 0;
		}
		if (strcmp(token[myi], "showdown/ ") == 0)
		{
			printf("message showdown\n");
			ROUND = 5;
			printf("showdown");
			return 0;
		}
		if (strcmp(token[myi], "pot-win/ ") == 0)
		{
			printf("message spot-win\n");
			printf("pot-win");
			return 0;//
		}

	}
	return 0;
}
/*���ַ�������߽�ȡn���ַ�*/
char * left(char *dst, char *src, int n)
{
	char *p = src;
	char *q = dst;
	int len = strlen(src);
	if (n>len) n = len;
	while (n--) *(q++) = *(p++);
	*(q++) = '\0';	//ָ���β
	return dst;
}

/*���ַ������м��ȡn���ַ�*/
char * mid(char *dst, char *src, int n, int m) /*nΪ���ȣ�mΪλ��*/
{
	char *p = src;
	char *q = dst;
	int len = strlen(src);
	if (n>len) n = len - m;    /*�ӵ�m�������*/
	if (m<0) m = 0;    /*�ӵ�һ����ʼ*/
	if (m>len) return NULL;
	p += m;
	while (n--) *(q++) = *(p++);
	*(q++) = '\0';//ָ���β
	return dst;
}
void decpoi(POKER myPoker, char *pokinfo)
{
	if (!strcmp(pokinfo, "J")) myPoker->number = 11;
	else if (!strcmp(pokinfo, "Q")) myPoker->number = 12;
	else if (!strcmp(pokinfo, "K")) myPoker->number = 13;
	else if (!strcmp(pokinfo, "A")) myPoker->number = 14;
	else myPoker->number = atoi(pokinfo);
}
void decol(POKER myPoker, char *pokinfo)
{
	if (!strcmp(pokinfo, "SPADES")) myPoker->color = SPADES;
	if (!strcmp(pokinfo, "HEARTS")) myPoker->color = HEARTS;
	if (!strcmp(pokinfo, "CLUBS"))	myPoker->color = CLUBS;
	if (!strcmp(pokinfo, "DIAMONDS")) myPoker->color = DIAMONDS;
}
HANDCARDS checkHold(char *str1, char *str2)
{
	/*-------�õ�������Ϣ--------*/
	printf("1worininainairiniyeye");
	char *first[2];
	char *second[2];
	first[0] = strtok(str1, " ");
	first[1] = strtok(NULL, " ");
	printf("���ƣ�%s %s", first[0], first[1]);
	printf("2worininainairiniyeye");
	second[0] = strtok(str2, " ");
	second[1] = strtok(NULL, " ");
	printf("���ƣ�%s %s\n", second[0], second[1]);
	/*---------------------------*/
	handpk = mhandpk;
	decol(handpk, first[0]);
	decol(handpk + 1, second[0]);
	decpoi(handpk, first[1]);
	decpoi(handpk + 1, second[1]);
	int mynum1 = handpk->number;
	int mynum2 = (handpk + 1)->number;
	/*--------holdȦ����ǰ��ע---------*/
	if (mynum1 == mynum2)	{ return PAIRS; printf("3worininainairiniyeye"); }

	else if (max(mynum1, mynum2) >= 11)	 { return HIGHCARD; printf("4worininainairiniyeye"); }

	else if (handpk->color == (handpk + 1)->color) { return SAMECOLOR; printf("5worininainairiniyeye"); }
	else return DEFAULT;
}
HANDCARDS checkFlop(char *str1, char *str2, char *str3)
{
	/*-------��������ϲ��õ�?µ��������?-------*/
	const char *flopdel = " ";
	char *first[2];
	char *second[2];
	char *third[2];
	first[0] = strtok(str1, flopdel);
	first[1] = strtok(NULL, flopdel);
	printf("���ƣ�%s %s", first[0], first[1]);
	/*---------------------------*/
	second[0] = strtok(str2, flopdel);
	second[1] = strtok(NULL, flopdel);
	/*---------------------------*/
	third[0] = strtok(str3, flopdel);
	third[1] = strtok(str3, flopdel);
	/*---------------------------*/

	decol(fpok, first[0]);
	decol(fpok + 1, second[0]);
	decol(fpok + 2, third[0]);
	/*---------------------------*/
	decpoi(fpok, first[1]);
	decpoi(fpok + 1, second[1]);
	decpoi(fpok + 2, third[1]);
	int pnum1 = fpok->number;
	int pnum2 = (fpok + 1)->number;
	int pnum3 = (fpok + 2)->number;
	/*--------holdȦ����ǰ��ע---------*/
	HANDCARDS tmpflop = judge(handpk, fpok, NULL, NULL);

	return tmpflop;
}
HANDCARDS checkTurn(char *str)
{
	char *first[2];
	first[0] = strtok(str, " ");
	first[1] = strtok(NULL, " ");
	printf("���ƣ�%s %s", first[0], first[1]);

	/*---------------------------*/
	decol(tpok, first[0]);
	/*---------------------------*/
	decpoi(tpok, first[1]);
	int tnum = tpok->number;
	HANDCARDS tmpturn = judge(handpk, fpok, tpok, NULL);

	return tmpturn;
}
HANDCARDS checkRiver(char *str)
{
	char *first[2];
	first[0] = strtok(str, " ");
	first[1] = strtok(NULL, " ");
	printf("���ƣ�%s %s", first[0], first[1]);

	/*---------------------------*/
	decol(rpok, first[0]);
	/*---------------------------*/
	decpoi(rpok, first[1]);
	int rnum = rpok->number;
	HANDCARDS tmpriver = judge(handpk, fpok, tpok, rpok);
	return tmpriver;
}
HANDCARDS judge(POKER h_pok, POKER f_pok, POKER t_pok, POKER r_pok)
{
	//����ж�����
	HANDCARDS TMP = DEFAULT;//Ĭ��Ϊ��С����
	int a[7] = { h_pok->number, (h_pok + 1)->number,
		f_pok->number, (f_pok + 1)->number, (f_pok + 2)->number,
		t_pok->number, r_pok->number };
	COLOR b[7] = { h_pok->color, (h_pok + 1)->color,
		f_pok->color, (f_pok + 1)->color, (f_pok + 2)->color,
		t_pok->color, r_pok->color };
	/*-----------------������----------------*/
	int len = sizeof(a);
	int i;
	for (i = 1; i < len; i++)//	��ѭ��
	{
		int itemp, j;
		for (j = 0; j >= i; j--)
		{
			if (a[j]>a[j - 1])
			{
				itemp = a[j - 1];
				a[j - 1] = a[j];
				a[j] = itemp;
			}
		}
	}
	/*------------------����Ȧ-----------------*/
	if (t_pok == NULL)
	{
		/*---------------�ж�˳�ӣ�˫�ԣ�����������---------------*/
		int flag = 0, pairs = 0, tri = 0, sit = 0;
		if ((a[1] = (a[0] + a[2]) / 2) && (a[2] = (a[1] + a[3]) / 2) && (a[3] = (a[2] + a[4]) / 2))
			TMP = SHUNZI;
		int i_f;
		for (i_f = 1; i_f < 5; i_f++)
		{
			if (a[i_f] - a[i_f - 1] == 1)
			{
				flag = flag + 1;
			}
		}
		int j;
		for (j = 1; j < 5; j++)
		{
			if (a[j] == a[j - 1])
			{
				if ((j < 4 && a[j + 1] != a[j]) || (j == 4))
				{
					pairs = pairs + 1;
				}//�õ����Ӹ���
				else if (j < 3 && a[j + 1] == a[j] && a[j + 2] == a[j])
					sit = sit + 1;		 //�õ�����������
				else if (j < 4 && a[j + 1] == a[j])
					tri = tri + 1;
			}

		}
		if (pairs == 1)	TMP = PAIRS;
		if (pairs == 2)	TMP = TPAIRS;
		if (tri)	TMP = SANTIAO;
		if (sit)	TMP = SITIAO;
		/*-----------------˳��----------------*/
		if (flag == 4) TMP = SHUNZI;
		if (flag == 3) TMP = LKSHUNZI;
		else if ((pairs) && flag == 2 && (a[4] - a[0] == 4)) TMP = P_LKSHUNZI;
		else if ((!pairs) && flag == 2 && (a[3] - a[0] == 4 || a[4] - a[1] == 4)) TMP = LKSHUNZI;
		/*----------------ͬ��----------------*/
		int colf = 1, ic;
		for (ic = 1; ic <5; ic++)
		{
			if (b[ic] == b[ic - 1]) colf = colf + 1;
		}
		if (TMP == SHUNZI&&colf == 5) TMP = KING;
		else if (colf == 5) TMP = TONGHUA;
		if (colf == 4 && pairs != 0) TMP = P_LKTONGHUA;
	}
	/*------------------ת��Ȧ------------------*/
	if ((t_pok != NULL) && (r_pok == NULL))
	{
		/*---------------�ж�˳�ӣ�˫�ԣ�����������---------------*/
		int flag = 0, pairs = 0, tri = 0, sit = 0;
		int i;
		for (i = 1; i < 6; i++)
		{
			if (a[i] - a[i - 1] == 1)
			{
				flag = flag + 1;
			}
		}
		int j;
		for (j = 1; j < 6; j++)
		{
			if (a[j] == a[j - 1])
			{
				if ((j < 5 && a[j + 1] != a[j]) || (j == 4))
				{
					pairs = pairs + 1;
				}//�õ����Ӹ���
				else if (j < 4 && a[j + 1] == a[j] && a[j + 2] == a[j])
					sit = 1;		 //�õ�����������
				else if (j < 5 && a[j + 1] == a[j])
					tri = 1;
			}

		}
		if (pairs == 1)	TMP = PAIRS;
		if (pairs == 2)	TMP = TPAIRS;
		if (tri)	TMP = SANTIAO;
		if (sit)	TMP = SITIAO;
		/*------------------˳��------------------*/
		if (flag == 4) TMP = SHUNZI;
		if (flag == 3) TMP = LKSHUNZI;
		else if ((pairs == 1) && flag == 2)
		{
			int tip, k;
			for (k = 1; k < 6; k++)
			{
				if (a[k] = a[k - 1]) { tip = k - 1; }
			}
			if (tip == 0 && a[tip + 2] - a[tip] != 1 && a[5] - a[2] == 4)
				TMP = P_LKSHUNZI;
			if (tip == 4 && a[tip] - a[tip - 1] != 1 && a[3] - a[0] == 4)
				TMP = P_LKSHUNZI;
			if (a[4] - a[0] == 4 && a[5] - a[1] == 4)
				TMP = P_LKSHUNZI;
		}
		else if ((pairs == 2) && flag == 2 && (a[5] - a[0] <= 4)) TMP = SPECIAL;
		else if ((!pairs) && flag == 2 && (a[3] - a[0] == 4 || a[4] - a[1] == 4 || a[5] - a[2] == 4)) TMP = LKSHUNZI;
		/*----------------ͬ��-----------------*/
		int colf = 1, ic;
		for (ic = 1; ic <6; ic++)
		{
			if (b[ic] == b[ic - 1]) colf = colf + 1;
		}
		if (TMP == SHUNZI&&colf == 5) TMP = KING;
		else if (colf == 5) TMP = TONGHUA;
		if (colf == 4 && pairs == 1) TMP = P_LKTONGHUA;
		if (colf == 4 && pairs == 2)TMP = SPECIAL;
	}
	/*-----------------����Ȧ------------------*/
	if ((t_pok != NULL) && (r_pok != NULL))
	{
		/*---------------�ж�˳�ӣ�?��ԣ����������?--------------*/
		int flag = 0, pairs = 0, tri = 0, sit = 0;
		int i;
		for (i = 1; i < 7; i++)
		{
			if (a[i] - a[i - 1] == 1)
			{
				flag = flag + 1;
			}
		}
		int j;
		for (j = 1; j < 7; j++)
		{
			if (a[j] == a[j - 1])
			{
				if ((j < 6 && a[j + 1] != a[j]) || (j == 4))
				{
					pairs = pairs + 1;
				}//�õ����Ӹ���
				else if (j < 5 && a[j + 1] == a[j] && a[j + 2] == a[j])
					sit = 1;		 //�õ�����������
				else if (j < 6 && a[j + 1] == a[j])
					tri = 1;
			}

		}
		if (pairs == 1)	TMP = PAIRS;
		if (pairs == 2)	TMP = TPAIRS;
		if (tri)	TMP = SANTIAO;
		if (sit)	TMP = SITIAO;
		/*------------------˳��------------------*/
		if (flag == 4) TMP = SHUNZI;
		/*------------------ͬ����ͬ��˳------------------*/
		int colf = 1, ic;
		for (ic = 1; ic <5; ic++)
		{
			if (b[ic] == b[ic - 1]) colf = colf + 1;
		}
		if (TMP == SHUNZI&&colf == 5) TMP = KING;
		else if (colf == 5) TMP = TONGHUA;
	}
	return TMP;
}
void myaction(struct info inf[], int pot, int rd, int size)
{
	int all_in_per = 0;
	int max_jet;
	srand((unsigned)time(NULL));
	switch (rd)
	{
	case 1:
	{		  /*-------------holdȦ-------------*/
			  /*----------------��������������Ϊ�м�ʱall_in----------------*/
			  if (inf[size - 1].jetton <= 250 || inf[size - 1].money<200) strcpy(SendBuff, "all_in");
			  printf("�ұ�?Ƶ������?");

			  //�õ��Լ�����,��������ʱall_in
			  int last = 1;
			  int maxmoney, maxjetton, mymoney;
			  int place;
			  if (hold_rd == 2)
			  {
				  mymoney = inf[size - 1].money;
				  int i;
				  for (i = 0; i < size - 1; i++)
				  {
					  if (inf[size - 1].jetton>inf[i].jetton)
					  {
						  last = 0;
						  break;
					  }
				  }
			  }
			  int j;
			  for (j = 0; j < size - 1; j++)
			  {
				  if (inf[j].money>inf[j + 1].money)	maxmoney = inf[j].money;
				  if (inf[j].jetton > inf[j + 1].jetton)  maxjetton = inf[j].jetton;
			  }
			  if (!last && ((maxjetton - inf[size - 1].jetton)>3000 || (maxmoney - inf[size - 1].money)>4000))
			  {
				  strcpy(SendBuff, "all_in");
				  printf("�ұ��Ƶ�������2");
			  }
			  //��inf��Ǯ���򣬲��õ��Լ�������;
			  int i_p;
			  for (i_p = 1; i_p < size; i_p++)//	��ѭ��
			  {
				  int itemp, j_p;
				  for (j_p = 0; j_p >= i_p; j_p--)
				  {
					  if (inf[j_p].money>inf[j_p - 1].money)
					  {
						  itemp = inf[j_p - 1].money;
						  inf[j_p - 1].money = inf[j_p].money;
						  inf[j_p].money = itemp;
					  }
				  }
			  }
			  int k;
			  for (k = 0; k < size; k++)
			  {
				  if (mymoney == inf[k].money)
				  {
					  place = k;
				  }
			  }
			  if (size >= 5 && place >= (size / 2 + 1) && handpk->number == (handpk + 1)->number)
			  {
				  if (handpk->number >= 11)
				  {
					  strcpy(SendBuff, "all_in");
					  printf("�ұ��Ƶ�������3");
				  }

			  }
			  /*�������all-in�������治��*/
			  int m;
			  for (m = 0; m < size; m++)
			  {
				  while (m < size - 1)
				  {
					  max_jet = max(inf[m].jetton, inf[m + 1].jetton);
				  }
				  if (strcmp(inf[m].action, "all_in"))
				  {
					  all_in_per += 1;
				  }
			  }
			  if (all_in_per != 0 && max_jet >= 300)
			  {
				  if (paimian != HIGH)
				  {
					  strcpy(SendBuff, "fold");
				  }
				  else if (paimian == HIGH && handcards == PAIRS)
					  strcpy(SendBuff, "call");
				  else 	 strcpy(SendBuff, "fold");
			  }
			  else if (all_in_per != 0 && max_jet < 300) strcpy(SendBuff, "call");
			  else if (all_in_per == 0)
			  {
				  //�õ�raise���������
				  char sz[10];
				  int randj;

				  if (handcards == PAIRS && paimian == HIGH)
				  {
					  if (hold_rd>4)
					  {
						  strcpy(SendBuff, "call");
					  }
					  randj = rand() % 200 + 100;
					  sprintf(sz, "%d", randj);
					  strcpy(SendBuff, "raise ");
					  strcat(SendBuff, sz);

				  }
				  else if (handcards == PAIRS)
				  {
					  //strcpy(SendBuff, "raise 500");
					  randj = rand() % 100 + 10;
					  sprintf(sz, "%d", randj);
					  strcpy(SendBuff, "raise ");
					  strcat(SendBuff, sz);
				  }
				  else if (handcards != DEFAULT)	 strcpy(SendBuff, "call");
				  else if (max_jet <= 300)		strcpy(SendBuff, "call");
				  else strcpy(SendBuff, "fold");
			  }
			  SendLen = send(socket_client, SendBuff, 100, 0);
			  if (SendLen < 0)
			  {
				  printf("myaction sending failed");
			  }
			  //���action����,���ƴ�С�����жϣ��Ϳ�ʼ���ԡ�����??��Ϊ�ƶ����Ĳ�?��*��ͳ��ÿ������fold��raise?�all_in����,�ó������Ը�
			  //��ɴ�����Թ��������д����c�ļ����궨��д��ͷ�ļ����档
			  //GOD BLESS!   check������û���ء�����
			  //ͳ��ÿ����Ȧ������Ȧflop,turn,river?
			  break;
	}
	case 2:
	{
			  /*-------------flopȦ-------------*/
			  int i;
			  for (i = 0; i < size; i++)
			  {
				  while (i < size - 1)
				  {
					  max_jet = max(inf[i].jetton, inf[i + 1].jetton);
				  }
				  if (strcmp(inf[i].action, "all_in"))
				  {
					  all_in_per += 1;
				  }
			  }
			  /*---------------�ж����ͻ�ʤ����-----------*/
			  double prob = 0.15;
			  char sz[10];//��ע��Ŀ�����������
			  int randj;
			  switch (handcards)
			  {
			  case SHUNZI:  prob = 0.9; break;
			  case TONGHUA: prob = 0.95; break;
			  case KING: prob = 1.0; break;
			  case HULU: prob = 1.0; break;
			  case SITIAO: prob = 1.0; break;
			  case TPAIRS:prob = 0.5; break;
			  case PAIRS:prob = 0.2; break;
			  case P_LKSHUNZI: prob = 0.55; break;
			  case P_LKTONGHUA: prob = 0.6; break;
			  case SPECIAL: prob = 0.7; break;
			  default:	prob = 0.25; break;
			  }
			  /*-----------------------------------------*/
			  if (all_in_per != 0 && max_jet >= 300)
			  {
				  if (prob >= 0.7 && (handcards == TPAIRS&&paimian == HIGH))
				  {
					  strcpy(SendBuff, "call");
				  }
				  else strcpy(SendBuff, "fold");
			  }
			  else if (all_in_per != 0 && max_jet < 300) strcpy(SendBuff, "call");
			  else if (all_in_per == 0)
			  {

				  if (handcards == TPAIRS && paimian == HIGH)
				  {
					  if (flop_rd>3)
					  {
						  strcpy(SendBuff, "call");
					  }
					  else strcpy(SendBuff, "raise 200");
				  }
				  else if (prob >= 0.7)
				  {
					  randj = rand() % 550 + 450;
					  sprintf(sz, "%d", randj);
					  strcpy(SendBuff, "raise ");
					  strcat(SendBuff, sz);
					  //strcpy(SendBuff, "raise 500");
				  }
				  else if (prob >= 0.5)	 strcpy(SendBuff, "call");
				  else if (max_jet <= 250)		strcpy(SendBuff, "call");
				  else strcpy(SendBuff, "fold");
			  }
			  //���Լ���������ֺ���.
			  SendLen = send(socket_client, SendBuff, 100, 0);
			  if (SendLen<0)
			  {
				  printf("myaction sending failed");
			  }
			  break;
	}
	case 3:
	{
			  /*-------------turnȦ-------------*/
			  int i;
			  for (i = 0; i < size; i++)
			  {
				  while (i < size - 1)
				  {
					  max_jet = max(inf[i].jetton, inf[i + 1].jetton);
				  }
				  if (strcmp(inf[i].action, "all_in"))
				  {
					  all_in_per += 1;
				  }
			  }
			  /*---------------�ж����ͻ�ʤ����-----------*/
			  double prob = 0.15;
			  char sz[10];
			  int randj;//��ע��Ŀ�����������
			  switch (handcards)
			  {
			  case SHUNZI:  prob = 0.9; break;
			  case TONGHUA: prob = 0.95; break;
			  case KING: prob = 1.0; break;
			  case HULU: prob = 1.0; break;
			  case SITIAO: prob = 1.0; break;
			  case TPAIRS:prob = 0.5; break;
			  case PAIRS:prob = 0.2; break;
			  case P_LKSHUNZI: prob = 0.55; break;
			  case P_LKTONGHUA: prob = 0.6; break;
			  case SPECIAL: prob = 0.7; break;
			  default:	prob = 0.25; break;
			  }
			  /*-----------------------------------------*/
			  if (all_in_per != 0 && max_jet >= 300)
			  {
				  if (prob >= 0.7)
				  {
					  strcpy(SendBuff, "call");
				  }
				  else strcpy(SendBuff, "fold");
			  }
			  else if (all_in_per != 0 && max_jet < 300) strcpy(SendBuff, "call");
			  else if (all_in_per == 0)
			  {

				  if (handcards == TPAIRS && paimian == HIGH)
				  {
					  if (flop_rd>3)
					  {
						  strcpy(SendBuff, "call");
					  }
					  else strcpy(SendBuff, "raise 200");
				  }
				  else if (prob >= 0.7)
				  {
					  randj = rand() % 550 + 450;
					  sprintf(sz, "%d", randj);
					  strcpy(SendBuff, "raise ");
					  strcat(SendBuff, sz);
					  //strcpy(SendBuff, "raise 500");
				  }
				  else if (prob >= 0.5)	 strcpy(SendBuff, "call");
				  else if (max_jet <= 250)		strcpy(SendBuff, "call");
				  else strcpy(SendBuff, "fold");
			  }
			  //���Լ���������ֺ���.
			  SendLen = send(socket_client, SendBuff, 100, 0);
			  if (SendLen<0)
			  {
				  printf("myaction sending failed");
			  }
			  break;
	}
	case 4:
	{
			  /*-------------riverȦ-------------*/
			  int i;
			  for (i = 0; i < size; i++)
			  {
				  while (i < size - 1)
				  {
					  max_jet = max(inf[i].jetton, inf[i + 1].jetton);
				  }
				  if (strcmp(inf[i].action, "all_in"))
				  {
					  all_in_per += 1;
				  }
			  }
			  /*---------------�ж����ͻ�ʤ����-----------*/
			  double prob = 0.15;
			  char sz[10];
			  int randj;
			  switch (handcards)
			  {
			  case SHUNZI:  prob = 0.9; break;
			  case TONGHUA: prob = 0.95; break;
			  case KING: prob = 1.0; break;
			  case HULU: prob = 1.0; break;
			  case SITIAO: prob = 1.0; break;
			  case TPAIRS:prob = 0.5; break;
			  case PAIRS:prob = 0.2; break;
			  default:	break;
			  }
			  /*-----------------------------------------*/
			  if (all_in_per != 0 && max_jet >= 300)
			  {
				  if (prob >= 0.7)
				  {
					  strcpy(SendBuff, "call");
				  }
				  else strcpy(SendBuff, "fold");
			  }
			  else if (all_in_per != 0 && max_jet < 300) strcpy(SendBuff, "call");
			  else if (all_in_per == 0)
			  {

				  if (handcards == TPAIRS && paimian == HIGH)
				  {
					  if (flop_rd>3)
					  {
						  strcpy(SendBuff, "call");
					  }
					  else strcpy(SendBuff, "raise 200");
				  }
				  else if (prob >= 0.7)
				  {
					  randj = rand() % 550 + 450;
					  sprintf(sz, "%d", randj);
					  strcpy(SendBuff, "raise ");
					  strcat(SendBuff, sz);
					  //strcpy(SendBuff, "raise 400");
				  }
				  else if (prob >= 0.5)	 strcpy(SendBuff, "call");
				  else if (max_jet <= 250)		strcpy(SendBuff, "call");
				  else strcpy(SendBuff, "fold");
			  }
			  //���Լ���������ֺ���.
			  SendLen = send(socket_client, SendBuff, 100, 0);
			  if (SendLen<0)
			  {
				  printf("myaction sending failed");
			  }
			  break;
	}
	case 5:	break;
	default:
		break;
	}

}
void alyOthers(char **p)
{
	char **info = p;
}
int m_socket_id = -1;

/* ����server����Ϣ */
int GetServerMessage(const char* buffer)
{
	printf("Recieve Data From Server(%s)\n", buffer);
	if (!strcmp(buffer, "game-over \n"))
		return -1;
	else if (sizeof(buffer) > 0)
	{
		return 0;
	}
	else return -1;
}

int main(int argc, char *argv[])
{

	if (argc != 6)
	{
		printf("Usage: ./%s server_ip server_port my_ip my_port my_id\n", argv[0]);
		return -1;
	}
	char RecieveBuff[500];
	int RecieveLen;
	/* ��ȡ������� */
	in_addr_t server_ip = inet_addr(argv[1]);
	in_port_t server_port = htons(atoi(argv[2]));
	in_addr_t my_ip = inet_addr(argv[3]);
	in_port_t my_port = htons(atoi(argv[4]));
	int my_id = atoi(argv[5]);

	/* ����socket */
	socket_client = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_client < 0)
	{
		printf("init socket failed!\n");
		return -1;
	}

	/* ����socketѡ���ַ�ظ�ʹ�ã���ֹ����������˳����´�����ʱbindʧ�� */
	int is_reuse_addr = 1;
	setsockopt(socket_client, SOL_SOCKET, SO_REUSEADDR, (const char*)&is_reuse_addr, sizeof(is_reuse_addr));

	/* ��ָ��ip��port����Ȼ�ᱻserver�ܾ� */
	struct sockaddr_in my_addr;
	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = my_ip;
	my_addr.sin_port = my_port;
	if (bind(socket_client, (struct sockaddr*)&my_addr, sizeof(my_addr)))
	{
		printf("bind failed!\n");
		return -1;
	}

	/* ����server */
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = server_ip;
	server_addr.sin_port = server_port;
	while (connect(socket_client, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		/* sleep 100ms, Ȼ�����ԣ���֤����server�����Ǻ��𣬶����������� */
		usleep(100 * 1000);
	}

	/* ��serverע�� */
	char reg_msg[50] = { '\0' };
	snprintf(reg_msg, sizeof(reg_msg)-1, "reg: %d %s \n", my_id, "cg4561232007");
	send(socket_client, reg_msg, strlen(reg_msg) + 1, 0);
	/* ����server��Ϣ��������Ϸ */
	while (1)
	{
		int ReceiveLen = 1;
		ReceiveLen = recv(socket_client, ReceiveBuff, sizeof(ReceiveBuff)-1, 0);
		if (ReceiveLen < 0)
		{
			printf("Receive failed!\n");
			break;
		}
		else
		{
			/* on_server_message����-1�������յ�game over��Ϣ����������ѭ�����ر�socket����ȫ�˳����� */
			if (-1 == GetServerMessage(ReceiveBuff))
			{
				printf("game over!\n");
				break;
			}
			onMessageCheck(ReceiveBuff);
		}
	}

	/* �ر�socket */
	close(socket_client);

	return 0;
}