#include <stdio.h>
#include <usb.h>
#include <errno.h>
#include <sqlite3.h>
#include <time.h>

/*
 * Temper.c by Robert Kavaler (c) 2009 (relavak.com)
 * All rights reserved.
 *
 * Modified by Sylvain Leroux (c) 2012 (sylvain@chicoree.fr)
 *
 * Modified by Michael Robinson (c) 2017 (michael@robinson-west.com) 
 *
 * Modified by Anthony Bateman (c) 2017 (tmbateman@hotmail.com)
 *
 * Temper driver for linux. This program can be compiled either as a library
 * or as a standalone program (-DUNIT_TEST). The driver will work with some
 * TEMPer usb devices from RDing (www.PCsensor.com).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Robert kavaler BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include "comm.h"

#if !defined TEMPER_TIMEOUT
#define TEMPER_TIMEOUT 1000	/* milliseconds */
#endif

#if !defined TEMPER_DEBUG
#define TEMPER_DEBUG 0
#endif




char * create_timestamp_human_readable();
int create_timestamp();




int main(int argc, char *argv[])
{  //***************************************************************************
   // This is a proof of concept of creating a sqlite3 database table in C code 
   //   and updating the table with individual sensor data.
   //***************************************************************************
    if ( argc < 3 )
    {
         printf ("%s\n","Usage: temper <db_filename> <hours>");

         return 1; // Not enough command line arguments...
    }

    char * filename = argv[1]; // Name of the database file.

    int hours=atoi(argv[2]); // How many hours to gather data.

    printf("%s %s %s %d\n","filename:",filename,"hours:",hours);

    // Got the database filename and hours from the command line...
    //**************************************************************************

    //--------------------------------------------------------------------------
    Temper *t=NULL;                     // A temper2 temperature sensing device.

    int ret=0;                          // Return value from TemperGetData.
    long current_time=0;                // Unintelligable time in seconds.
    long start_time=create_timestamp(); // Need to remember start for timing.
    long end_time=start_time;           // Future timestamp we need to reach.
    sqlite3 *db=NULL;                   // Handle for the sqlite3 database.
    char *err_msg = 0;                  // An error string.
    char sql[160];                      // For Structured Query Language commands.
    unsigned char buf[8];
    char sn[80];                        // Temper device serial number.

    // Set the end time based on number of hours to run.
    end_time = end_time + (hours * 60 * 60);

    // Create database.
    int rc = sqlite3_open(filename, &db);  
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "Cannot open db: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 2;
    }
    // Defined some variables and created the database...
    //--------------------------------------------------------------------------
   
    // *************************************************************************
    // Build the table if it doesn't yet exist
    sprintf( sql,"%s%s","CREATE TABLE sensors"
                ,"(Id INT, timestamp INT,inner_temp FLOAT, outer_temp FLOAT);");
    
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) 
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        return 3;
    }
    // *************************************************************************

    // Initialize the USB bus...
    usb_set_debug(0);
    usb_init();
    usb_find_busses();
    usb_find_devices();

    do
    {
        int device_count = TemperCount(); // Number of devices to loop through.
  
        do
        {
            --device_count; // Loop from max device to least device.
            current_time = create_timestamp();

            // WARNING, if existing temper device need to deallocate it.
            if(t)
            {
                TemperFree(t);
                t=NULL;
            }

	    t = TemperCreateFromDeviceNumber
                (device_count, TEMPER_TIMEOUT, TEMPER_DEBUG);

	    if(!t) 
            {
                if (db) // Don't forget to close the database if open.
                { sqlite3_close(db); }
		perror("TemperCreate");
		return -1;
	    }

	    TemperSendCommand8
            (t, 0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00);

            if (0)
            {
	        TemperInterruptRead(t, buf, sizeof(buf));
            }
            else
            {
	        TemperData data[2];
	        const unsigned int count = sizeof(data)/sizeof(TemperData);
	        ret = TemperGetData(t,data,count);
	        printf("RET: %d", ret);

	        for (unsigned i = 0; i < count; ++i) 
                {
	             printf(";%f %s", 
				data[i].value, 
				TemperUnitToString(data[i].unit)
                           );
                }

                // Build the SQL insert string:
	        sprintf(sql, "INSERT INTO sensors VALUES(%d,%ld,%f,%f);",
                              device_count,
                              current_time,
                              data[0].value,
                              data[1].value
                       );

                // Save data to database.
                rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	        if (rc != SQLITE_OK ) 
                {
	            fprintf(stderr, "SQL error: %s\n", err_msg);	
        	    sqlite3_free(err_msg);
        	    sqlite3_close(db);
                    if (t)
                    { TemperFree(t); }

                    return 4;
	        }

	        printf("\nsql: %s\n",sql);

                sql[0]='\0';
	        TemperGetSerialNumber(t, sn, sizeof(sn));
	        printf(";%s;%s\n", t->product->name, sn);
	        printf("%s\n",create_timestamp_human_readable());
            }

        } while ( device_count > 0 );

   } while ( current_time < end_time );

   sleep(10);
   sqlite3_close(db);

   if (t)
   { TemperFree(t); }

   return 0;
}



// Return human readable current date and time...
char * create_timestamp_human_readable()
{
       time_t current_time;
       char * buffer;
       current_time = time(NULL);
       buffer = ctime(&current_time);
       return buffer;
}



// Return integer non human readable date and time...
int create_timestamp()
{
     time_t current_time;
     current_time = time(NULL);
     return current_time;
}
