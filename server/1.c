/*#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

struct last_modified {
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
};

void f(struct last_modified* res, time_t data) {
    struct tm* timeinfo;
    timeinfo = localtime(&data);

    res->year = timeinfo->tm_year + 1900;
    res->month = timeinfo->tm_mon + 1;
    res->day = timeinfo->tm_mday;
    res->hour = timeinfo->tm_hour;
    res->min = timeinfo->tm_min;
    res->sec = timeinfo->tm_sec;
}

int main() {
	struct stat fst;
	stat("index.html", &fst);
	printf("%ld\n", fst.st_mtim.tv_sec);
	time_t data = fst.st_mtim.tv_sec;
	struct last_modified lm = {};
	f(&lm, data);
	char str[64] = "";
	sprintf(str, "%d.%d.%d  %d:%d:%d\r\n", lm.year, lm.month, lm.day, lm.hour, lm.min, lm.sec);
	printf("%s", str);
	return 0;
}

*/
