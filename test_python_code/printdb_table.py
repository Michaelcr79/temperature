import sqlite3 as lite
import sys
from datetime import datetime
import tzlocal

def readableTimeConvert(timeStamp):
        local_timezone=tzlocal.get_localzone()
        timeStamp=datetime.fromtimestamp(timeStamp,local_timezone)
        return str(timeStamp)
        
con = None

try:
    f = open('index.html','w')
    con = lite.connect('temper.sqlite3')
    cur = con.cursor()
    cur.execute('SELECT SQLITE_VERSION()')
    data = cur.fetchone()
    f.write("<html>\n")
    f.write("<head>\n")
    f.write("<title>Print out a database.</title>\n")
    f.write("</head>\n")
    f.write("<body>\n")
    f.write("<table border=2 cellpadding=10 cellspacing=10>\n")
    f.write("<tr>\n")
    f.write("<th>Sensor</th>\n")
    f.write("<th>Time Stamp</th>\n")
    f.write("<th>Inner Temperature</th>\n")
    f.write("<th>Outer Temperature</th>\n")
    f.write("</tr>\n")

    cur.execute('SELECT * from sensors')
    data = cur.fetchone()
    sensor,timeStamp,innerTemp,outerTemp=data
    while data:
        f.write("<tr>\n")
        f.write("<td>")
        f.write(str(sensor))
        f.write("</td>\n")
        f.write("<td>")
        f.write(readableTimeConvert(timeStamp))
        f.write("</td>\n")
        f.write("<td>")
        f.write(str(innerTemp))
        f.write("</td>\n")
        f.write("<td>")
        f.write(str(outerTemp))
        f.write("</td>\n")
        f.write("</tr>\n")
        data=cur.fetchone()
        if data:
            sensor,timeStamp,innerTemp,outerTemp=data
    f.write("</ul>\n")
    f.write("</body>\n")
    f.write("</html>\n")
    f.closed

except lite.Error:
    print("Error %s" % lite.Error.args[0])
    f.closed
    sys.exit(1)

finally:
    if con:
        con.close()
