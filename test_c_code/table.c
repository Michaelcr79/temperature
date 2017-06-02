#include<stdio.h>
#include<sqlite3.h>

int main(void)
{
    sqlite3 *db;
    char *err_msg = 0;

    int rc = sqlite3_open("test.db", &db);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }

    char *sql = "DROP TABLE IF EXISTS Cars;"
                "CREATE TABLE Cars(Id INT, Name TEXT, Price INT);" 
                "Insert into Cars VALUES(1, 'Audi', 52642);"
                "Insert into Cars VALUES(2, 'Mercedes', 57127);"
                "Insert into Cars VALUES(3, 'Skoda', 9000);"
                "Insert into Cars VALUES(4, 'Volvo', 29000);"
                "Insert into Cars VALUES(5, 'Bentley', 350000);"
                "Insert into Cars VALUES(6, 'Citroen', 21000);"
                "Insert into Cars VALUES(7, 'Hummer', 41400);"
                "Insert into Cars VALUES(8, 'Volkswagon', 21600);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if ( rc != SQLITE_OK )
    {
         fprintf(stderr, "SQL error: %s\n", err_msg);

         sqlite3_free(err_msg);
         sqlite3_close(db);

         return 1;
    }

    sqlite3_close(db);
 
    return 0;
}
