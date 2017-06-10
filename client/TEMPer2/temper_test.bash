#!/bin/bash
#
# Test different delays and time the whole run.  Assumes six sensors.

rm -f *.sqlite3 *.date

date > six.1.date ; ./temper six.1.sqlite3 1 >> six.1.date ; date >> six.1.date

date > six.2.date ; ./temper six.2.sqlite3 2 >> six.2.date ; date >> six.2.date

date > six.3.date ; ./temper six.3.sqlite3 3 >> six.3.date ; date >> six.3.date

ls -lh *.sqlite3
