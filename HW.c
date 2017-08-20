#include "/usr/include/mysql/mysql.h"
#include <wiringPi.h>
#include <string.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <stdint.h> 
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>


#define DB_HOST "DB_HOST" 
#define DB_USER "DB_USER" 
#define DB_PASS "DB_PASS" 
#define DB_NAME "DB_NAME" 
#define CHOP(x) x[strlen(x)] = ' ' 
#define MAX_TIMINGS 85 
#define DHT_PIN 15 // GPIO-14 
#define WATER_PIN 16 // GPIO-15
typedef struct Value {
	float t;
	float h;
	int water;
}SV;
void read_dht_data(SV *sv) {
	uint8_t laststate = HIGH;
	uint8_t counter = 0;
	uint8_t j = 0, i;
    int data[5] = { 0, 0, 0, 0, 0 };
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	/* pull pin down for 18 milliseconds */
	pinMode(DHT_PIN, OUTPUT);
	digitalWrite(DHT_PIN, LOW);
	delay(18);
	/* prepare to read the pin */
	pinMode(DHT_PIN, INPUT);
	/* detect change and read data */
	for (i = 0; i < MAX_TIMINGS; i++)
	{
		counter = 0;
		while (digitalRead(DHT_PIN) == laststate)
		{
			counter++;
			delayMicroseconds(1);
			if (counter == 255)
			{
				break;
			}
		}
		laststate = digitalRead(DHT_PIN);
		if (counter == 255)
			break;
		/* ignore first 3 transitions */
		if ((i >= 4) && (i % 2 == 0))
		{
			/* shove each bit into the storage bytes */
			data[j / 8] <<= 1;
			if (counter > 16)
				data[j / 8] |= 1;
			j++;
		}
	}
	/*
	* check we read 40 bits (8bit x 5 ) + verify checksum in the 
last byte
	* print it out if data is good
	*/
	if ((j >= 40) &&
		(data[4] == ((data[0] + data[1] + data[2] + data[3]) & 
0xFF)))
	{
		float h = (float)((data[0] << 8) + data[1]) / 10;
		if (h > 100)
		{
			h = data[0]; // for DHT11
		}
		float c = (float)(((data[2] & 0x7F) << 8) + data[3]) / 
10;
		if (c > 125)
		{
			c = data[2]; // for DHT11
		}
		if (data[2] & 0x80)
		{
			c = -c;
		}
		float f = c * 1.8f + 32;
		sv->h = h;
		sv->t = c;
	//	printf("Humidity = %.1f %% Temperature = %.1f *C (%.1f*F)\n", h, c, f);
	}
	else {
		sv->h = 0;
		sv->t = 0;
		printf("Data not good, skip\n");
	}
}

char* get_IP()
{
 int fd;
 struct ifreq ifr;

 fd = socket(AF_INET, SOCK_DGRAM, 0);

 /* I want to get an IPv4 IP address */
 ifr.ifr_addr.sa_family = AF_INET;

 /* I want IP address attached to "eth0" */
 strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);

 ioctl(fd, SIOCGIFADDR, &ifr);

 close(fd);

 /* display result */
 return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}


void sql_(SV *sv) {
	MYSQL *connection = NULL, conn;
	MYSQL_RES *sql_result;
	MYSQL_ROW sql_row;
	int query_stat;
	char ip_[20];
	char date_[25];
	float temperature;
	float humidity;
    int water;
	char query[255];
	mysql_init(&conn);
	connection = mysql_real_connect(&conn, DB_HOST,
		DB_USER, DB_PASS,
		DB_NAME, 3306,
		(char *)NULL, 0);
	if (connection == NULL)
	{
		fprintf(stderr, "Mysql connection error : %s",mysql_error(&conn));
	}
	/*query_stat = mysql_query(connection, "select * from TH");
	if (query_stat != 0)
	{
		fprintf(stderr, "Mysql query error : %s", 
mysql_error(&conn));
		return 1;
	}
	sql_result = mysql_store_result(connection);
	while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
	{
		printf("%+11s %-30s %0.1f %0.1f\n", sql_row[0], 
sql_row[1], sql_row[2], sql_row[3]);
	}
	mysql_free_result(sql_result);	*/
	time_t timer;
	struct tm *t;
	timer = time(NULL); // 현재 시각을 초 단위로 얻기
	t = localtime(&timer); // 초 단위의 시간을 분리하여 구조체에 넣기
	sprintf(date_, "%04d-%02d-%02d %02d:%02d:%02d",
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec
	);
	strcpy(ip_, get_IP());
	temperature = sv->t;
	humidity = sv->h;
    water = sv->water;
	CHOP(ip_);
	CHOP(date_);
	sprintf(query, "insert into rev_sensor values "
		"('%d', '%s', '%f', '%f', '%d', '%s')",
        0, date_, humidity, temperature, water, ip_);
	query_stat = mysql_query(connection, query);
	if (query_stat != 0)
	{
		fprintf(stderr, "Mysql humid query error : %s",mysql_error(&conn));
	}

	mysql_close(connection);
}
void getW(SV *sv) {
	int water;
	pinMode(WATER_PIN, INPUT);
    
	water = digitalRead(WATER_PIN);
	//sv.water = a;
    if(water ==1)
        sv->water = 0;
    else
        sv->water = 1;
    //printf("출력 값은 %d 입니다\n",a);
}
void main(void) {
	printf("H/T & WATER test\n");
	if(wiringPiSetup() == -1)
		printf("wiring problem\n");
    SV sv;
    while (1)
	{
		read_dht_data(&sv);
        getW(&sv);
		if(sv.t !=0.0 && sv.h != 0.0){
		sql_(&sv);
    //    printf("온도 값은 %f 습도 값은 %f 물의 여부는 %f 입니다\n",sv.t,sv.h,sv.water);
        }
        delay(1000); /* wait 1 seconds before next read */
        
    }
}
