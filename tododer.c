#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

sqlite3* db = NULL;
char* errBuffer = NULL;

void errorOut( int errCode ) {
    if (errCode != SQLITE_OK) {
        printf("ERROR: %s\n", sqlite3_errstr(errCode));
        printf("REASON: %s\n", errBuffer);
        sqlite3_close(db);
        exit(-1);
    }
    sqlite3_free(errBuffer);
}

int init ( void ) {
    int err = sqlite3_open("mydata.db", &db);
    errorOut(err);
}

void addGeneratorDB ( void ) {
    int err = sqlite3_exec(
        db,
        "CREATE TABLE IF NOT EXISTS generator ("
            "id INTEGER primary key autoincrement,"
            "content TEXT,"
            "createdTime INTEGER,"
            "endTime INTEGER"
        ")",
        NULL,
        NULL,
        &errBuffer
    );
    errorOut(err);
}

long parseTimestamp( const char* period ) {
/*
    Possible values:
    *s, *m, *h, *d, *M, *y
*/
    int l = strlen(period) - 1;
    char n[l+1];
    memcpy(n, period, l);
    n[l+1] = '\0';
    long ni = atoi(n);
    switch ( period[l] ) {
        case 's': return ni;
        case 'm': return ni * 60;
        case 'h': return ni * 60 * 60;
        case 'd': return ni * 60 * 60 * 24;
        case 'M': return ni * 2.628e+6;
        case 'y': return ni * 3.154e+7;
    }
    return 0;
}

void addGenerator ( const char* content, const char* period ) {
    long parsedTimestamp = parseTimestamp(period);
    if (parsedTimestamp == 0) {
        printf("Invalid timestamp provided");
        close(db);
        exit(-1);
    }
    time_t t = time(NULL);

    char t_buffer[sizeof(long)*8 + 1];
    char tp_buffer[sizeof(long)*8 + 1];

    sprintf(t_buffer, "%li", t);
    sprintf(tp_buffer, "%li", t + parsedTimestamp);

    char* sql = sqlite3_mprintf("INSERT INTO generator (content, createdTime, endTime) VALUES (%Q, %q, %q)", content, t_buffer, tp_buffer);
    int err = sqlite3_exec(db, sql, NULL, NULL, &errBuffer);
    sqlite3_free(sql);
    errorOut(err);
}

static int displayNotificationsCallback( void *unused, int count, char **data, char **columns ) {
    for (int i = 0; i < count; i++) {
        printf("%s\n", data[i]);
    }
    return(0);
}

void displayNotifications ( void ) {
    char t_buffer[sizeof(long)*8 + 1];
    sprintf(t_buffer, "%li", time(NULL));
    char* sql = sqlite3_mprintf("SELECT content FROM generator WHERE endTime > %q", t_buffer);
    int err = sqlite3_exec(db, sql, displayNotificationsCallback, NULL, &errBuffer);
    sqlite3_free(sql);
    errorOut(err);
}

int main ( int argc, char *argv[] ) {
    init();
    addGeneratorDB();
    for (int i = 0; i < argc; i++) {
        if ( (strcmp(argv[i], "-a") == 0) && argc > 3 ) {
            addGenerator(argv[i+1], argv[i+2]);
            i+=2;
        }
    }
    displayNotifications();
    sqlite3_close(db);
}