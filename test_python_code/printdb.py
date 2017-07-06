import sqlite3 as lite
import sys

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
        f.write("<ul>\n")
	
        cur.execute('SELECT * from sensors')

	data = cur.fetchone()
        while data:
            	f.write("<li>")
		f.write(str(data))
                f.write("</li>\n")
		data = cur.fetchone()

        f.write("</ul>\n")
        f.write("</body>\n")
        f.write("</html>\n")
	f.closed

except lite.Error, e:

	print "Error %s" % e.args[0]
        f.closed
	sys.exit(1)

finally:

	if con:
		con.close()
